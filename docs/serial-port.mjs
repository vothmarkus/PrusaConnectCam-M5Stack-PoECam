import { SerialPort } from "./vendor/web-serial-polyfill-1.0.15.mjs";

// Android may expose navigator.serial for Bluetooth only. Prefer WebUSB there
// even when the native API exists; leave desktop selection on Web Serial.
export function defaultConnection(nav = navigator) {
  const android = /Android/i.test(nav.userAgent || "") || nav.userAgentData?.platform === "Android";
  return android || (!nav.serial && nav.usb) ? "usb" : "serial";
}

// Call directly from a click handler, before any download or other awaited work.
export async function requestProgrammerPort(mode, nav = navigator) {
  if (mode === "serial") {
    if (!nav.serial?.requestPort) throw new Error("Web Serial fehlt. Bitte Chrome oder Edge verwenden.");
    return nav.serial.requestPort();
  }
  if (mode !== "usb") throw new Error("Unbekannte Verbindungsart.");
  if (!nav.usb?.requestDevice) throw new Error("WebUSB fehlt. Die Seite direkt in Chrome über HTTPS öffnen.");
  // The user's M5Stack downloader identifies as CH9102 (CDC-ACM), 1a86:55d4.
  // CP2104 and CH340 use different protocols and must not receive CDC requests.
  const device = await nav.usb.requestDevice({
    filters: [{ vendorId: 0x1a86, productId: 0x55d4 }]
  });
  if (device.vendorId !== 0x1a86 || device.productId !== 0x55d4) {
    throw new Error("Dieser USB-Zugang unterstützt den CH9102-Downloader. Andere Downloader bitte am PC verwenden.");
  }
  let port;
  try {
    port = new SerialPort(device);
  } catch (_) {
    throw new Error("Der Downloader bietet keine passende USB-CDC-Schnittstelle. Bitte das USB-Gerät prüfen.");
  }
  // esptool-js can change baud in place; avoid closing USB and toggling reset
  // lines while the ESP32 stub is running.
  port.setBaudRate = (baudRate) => port.reconfigure({ baudRate });
  return port;
}
