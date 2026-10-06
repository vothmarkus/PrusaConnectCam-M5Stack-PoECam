import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { webcrypto } from "node:crypto";
import test from "node:test";
import vm from "node:vm";
import { defaultConnection, requestProgrammerPort } from "../docs/serial-port.mjs";

function usbDevice() {
  const calls = [];
  const configuration = { configurationValue: 1, interfaces: [
    { interfaceNumber: 0, alternates: [{ interfaceClass: 2, endpoints: [
      { direction: "in", endpointNumber: 1, packetSize: 8, type: "interrupt" }
    ] }] },
    { interfaceNumber: 1, alternates: [{ interfaceClass: 10, endpoints: [
      { direction: "in", endpointNumber: 2, packetSize: 64, type: "bulk" },
      { direction: "out", endpointNumber: 2, packetSize: 64, type: "bulk" }
    ] }] }
  ] };
  const pending = [];
  let delivered = false;
  return {
    calls, vendorId: 0x1a86, productId: 0x55d4, opened: false,
    configurations: [configuration], configuration: null,
    async open() { calls.push(["open"]); this.opened = true; },
    async selectConfiguration(value) { assert.equal(value, 1); this.configuration = configuration; },
    async claimInterface(index) { calls.push(["claim", index]); },
    async controlTransferOut(setup, data) {
      calls.push(["control", setup, data ? new Uint8Array(data) : undefined]);
      return { status: "ok" };
    },
    async transferOut(endpoint, data) {
      calls.push(["write", endpoint, [...data]]);
      return { status: "ok", bytesWritten: data.length };
    },
    async transferIn(endpoint) {
      assert.equal(endpoint, 2);
      if (!delivered) {
        delivered = true;
        return { status: "ok", data: new DataView(Uint8Array.of(99, 0xc0, 1, 2, 0xc0, 99).buffer, 1, 4) };
      }
      return new Promise((resolve, reject) => pending.push(reject));
    },
    async close() {
      calls.push(["close"]); this.opened = false;
      for (const reject of pending.splice(0)) reject(new Error("USB closed"));
    }
  };
}

test("Android uses USB despite Bluetooth Web Serial; picker is called synchronously", async () => {
  const device = usbDevice();
  let requested = false;
  const nav = {
    userAgent: "Mozilla/5.0 (Linux; Android 16) Chrome/148.0",
    serial: { requestPort() { assert.fail("Bluetooth picker must not be used"); } },
    usb: { async requestDevice(options) {
      requested = true;
      assert.deepEqual(options.filters, [{ vendorId: 0x1a86, productId: 0x55d4 }]);
      return device;
    } }
  };
  assert.equal(defaultConnection(nav), "usb");
  const selected = requestProgrammerPort(defaultConnection(nav), nav);
  assert.equal(requested, true, "chooser must run before losing click activation");
  const port = await selected;
  assert.deepEqual(port.getInfo(), { usbVendorId: 0x1a86, usbProductId: 0x55d4 });
  assert.equal(device.calls.length, 0, "selection alone must not open/reset the device");
  assert.equal(defaultConnection({ userAgent: "Desktop", userAgentData: { platform: "Android" }, usb: {} }), "usb");
});

test("Desktop retains native Serial; missing USB never silently falls back to Bluetooth", async () => {
  const port = {};
  const nav = { userAgent: "Windows", serial: { requestPort: async () => port },
    usb: { requestDevice() { assert.fail("Desktop should use native serial"); } } };
  assert.equal(defaultConnection(nav), "serial");
  assert.equal(await requestProgrammerPort(defaultConnection(nav), nav), port);
  await assert.rejects(requestProgrammerPort("usb", { serial: nav.serial }), /WebUSB fehlt/);
  await assert.rejects(requestProgrammerPort("serial", {}), /Web Serial fehlt/);
});

test("CH9102 uses real CDC controls, byte streams and in-place baud changes", async () => {
  const device = usbDevice();
  const port = await requestProgrammerPort("usb", { usb: { requestDevice: async () => device } });
  await port.open({ baudRate: 115200 });
  assert.deepEqual(device.calls.filter(x => x[0] === "claim"), [["claim", 0], ["claim", 1]]);
  const lineCoding = device.calls.find(x => x[0] === "control" && x[1].request === 0x20);
  assert.equal(lineCoding[1].requestType, "class");
  assert.equal(lineCoding[1].index, 0);
  assert.deepEqual([...lineCoding[2]], [0x00, 0xc2, 0x01, 0x00, 0, 0, 8]);
  await port.setSignals({ dataTerminalReady: false, requestToSend: true });
  assert.equal(device.calls.at(-1)[1].request, 0x22);
  assert.equal(device.calls.at(-1)[1].value, 2);
  const before = device.calls.length;
  await port.setBaudRate(460800);
  assert.equal(device.calls.length, before + 1, "baud change must only send line coding");
  assert.equal(new DataView(device.calls.at(-1)[2].buffer).getUint32(0, true), 460800);
  const writer = port.writable.getWriter();
  await writer.write(Uint8Array.of(0xc0, 3, 4, 0xc0));
  writer.releaseLock();
  assert.deepEqual(device.calls.at(-1), ["write", 2, [0xc0, 3, 4, 0xc0]]);
  const reader = port.readable.getReader();
  assert.deepEqual([...(await reader.read()).value], [0xc0, 1, 2, 0xc0]);
  await reader.cancel(); reader.releaseLock();
  await port.close();
  assert.equal(device.opened, false);
  assert.equal(device.calls.filter(x => x[0] === "open").length, 1);
});

test("Unsupported adapter and cancelled selection never open USB", async () => {
  const device = usbDevice(); device.vendorId = 0x10c4; device.productId = 0xea60;
  await assert.rejects(requestProgrammerPort("usb", { usb: { requestDevice: async () => device } }), /CH9102/);
  assert.equal(device.calls.length, 0);
  await assert.rejects(requestProgrammerPort("usb", { usb: { requestDevice: async () => {
    throw new DOMException("Cancelled", "NotFoundError");
  } } }), { name: "NotFoundError" });
});

const html = await readFile(new URL("../docs/index.html", import.meta.url), "utf8");
const script = html.match(/<script type="module">([\s\S]*?)<\/script>/)[1]
  .replace(/^\s*import .*;$/gm, "");
const app = new Uint8Array(await readFile(new URL(
  "../ESP32_PrusaConnectCam_web/build/m5stack.esp32.m5stack_poe_cam/ESP32_PrusaConnectCam_web.ino.bin", import.meta.url)));
const firmwareDir = new URL("../ESP32_PrusaConnectCam_web/build/m5stack.esp32.m5stack_poe_cam/", import.meta.url);
const bootloader = new Uint8Array(await readFile(new URL("ESP32_PrusaConnectCam_web.ino.bootloader.bin", firmwareDir)));
const otaReset = new Uint8Array(await readFile(new URL("ota-reset.bin", firmwareDir)));
const assets = new Map([
  ["ESP32_PrusaConnectCam_web.ino.bin", app],
  ["ESP32_PrusaConnectCam_web.ino.bootloader.bin", bootloader],
  ["ota-reset.bin", otaReset]
]);

function page({ badHash = false, chip = "ESP32", failWrite = false, corruptAsset = "" } = {}) {
  const events = [];
  const elements = new Map();
  const element = id => {
    if (!elements.has(id)) elements.set(id, { value: id === "baud" ? "460800" : "serial", checked: false,
      disabled: false, textContent: "", listeners: {}, addEventListener(type, fn) { this.listeners[type] = fn; } });
    return elements.get(id);
  };
  const nav = { userAgent: "Android 16", serial: {}, usb: {} };
  class Transport { async disconnect() { events.push(["disconnect"]); } }
  class ESPLoader {
    constructor(options) { this.chip = { CHIP_NAME: chip }; events.push(["loader", options.baudrate]); }
    async main() { return chip; }
    async writeFlash(options) {
      events.push(["write", options]);
      if (failWrite) throw new Error("USB write interrupted");
    }
    async after(mode) { events.push(["reset", mode]); }
  }
  const context = vm.createContext({
    navigator: nav, document: { getElementById: element }, defaultConnection: () => defaultConnection(nav),
    requestProgrammerPort(mode) { events.push(["picker", mode]); return Promise.resolve({}); },
    Transport, ESPLoader, Uint8Array, console,
    crypto: badHash ? { subtle: { digest: async () => new ArrayBuffer(32) } } : webcrypto,
    async fetch(url) {
      events.push(["fetch"]);
      const name = url.split('/').at(-1);
      const data = assets.get(name).slice();
      if (name === corruptAsset) data[0] ^= 1;
      return { ok: true, arrayBuffer: async () => data.buffer };
    }
  });
  new vm.Script(script).runInContext(context);
  return { events, element };
}

test("Update verifies all files before writing and resets OTA boot selection last without touching NVS", async () => {
  const p = page();
  await p.element("update").listeners.click();
  assert.deepEqual(p.events[0], ["picker", "usb"]);
  assert.deepEqual(p.events[1], ["fetch"]);
  assert.deepEqual(p.events[4], ["loader", 115200]);
  const writes = p.events.filter(x => x[0] === "write").map(x => x[1]);
  assert.equal(writes.length, 2);
  const write = writes[0];
  assert.equal(write.fileArray[0].address, 0x10000);
  assert.equal(write.eraseAll, false);
  assert.deepEqual(write.fileArray[0].data, app);
  assert.equal(write.fileArray[1].address, 0x1000);
  assert.deepEqual(write.fileArray[1].data, bootloader);
  assert.equal(writes[1].fileArray[0].address, 0xe000);
  assert.deepEqual(writes[1].fileArray[0].data, otaReset);
  assert.equal(otaReset.length, 0x2000);
  assert(otaReset.every(x => x === 255));
  for (const operation of writes) {
    assert.equal(operation.eraseAll, false);
    for (const file of operation.fileArray) {
      const sectorEnd = Math.ceil((file.address + file.data.length) / 0x1000) * 0x1000;
      assert(file.address >= 0xe000 || sectorEnd <= 0x9000, "NVS 0x9000..0xdfff must not be erased");
    }
  }
  assert.equal(p.element("update").disabled, false);
});

test("Interrupted application write never resets OTA selection or reports success", async () => {
  const p = page({ failWrite: true }); await p.element("update").listeners.click();
  assert.equal(p.events.filter(x => x[0] === "write").length, 1);
  assert.equal(p.events.some(x => x[0] === "reset"), false);
  assert.match(p.element("status").textContent, /USB write interrupted/);
});

test("Corrupt bootloader or OTA reset file prevents every write", async () => {
  for (const corruptAsset of ["ESP32_PrusaConnectCam_web.ino.bootloader.bin", "ota-reset.bin"]) {
    const p = page({ corruptAsset }); await p.element("update").listeners.click();
    assert.equal(p.events.some(x => x[0] === "write" || x[0] === "loader"), false);
    assert.match(p.element("status").textContent, /SHA-256-Prüfung fehlgeschlagen/);
  }
});

test("Hash mismatch or wrong ESP32 variant prevents all writes", async () => {
  for (const options of [{ badHash: true }, { chip: "ESP32-S3" }]) {
    const p = page(options); await p.element("update").listeners.click();
    assert.equal(p.events.some(x => x[0] === "write"), false);
    if (options.badHash) assert.equal(p.events.some(x => x[0] === "loader"), false);
    assert.match(p.element("status").textContent, /FEHLER:/);
  }
});

test("Connection test never downloads or writes firmware; unconfirmed erase is blocked", async () => {
  const p = page(); await p.element("probe").listeners.click();
  assert.equal(p.events.some(x => x[0] === "write" || x[0] === "fetch"), false);
  assert.equal(p.events.some(x => x[0] === "reset"), true);
  const events = p.events.length;
  await p.element("install").listeners.click();
  assert.equal(p.events.length, events);
});
