// Uses the same native/WebUSB ports as the flasher, without starting esptool
// or sending a bootloader/reset sequence. Firmware logs use 115200 baud.
export class SerialMonitor {
  constructor({ onText = () => {}, onClose = () => {} } = {}) {
    this.onText = onText;
    this.onClose = onClose;
    this.port = null;
    this.running = false;
    this.writeTask = Promise.resolve();
  }

  async open(port) {
    if (this.port) throw new Error("Die Konsole ist bereits verbunden.");
    this.port = port;
    this.stopping = false;
    try {
      await port.open({ baudRate: 115200, dataBits: 8, stopBits: 1, parity: "none", flowControl: "none" });
      await port.setSignals({ dataTerminalReady: false, requestToSend: false });
      this.reader = port.readable.getReader();
    } catch (error) {
      try { await port.close(); } catch (_) {}
      this.port = null;
      throw error;
    }
    this.running = true;
    this.readTask = this.readLoop();
  }

  async readLoop() {
    const decoder = new TextDecoder();
    let failure;
    try {
      while (!this.stopping) {
        const { value, done } = await this.reader.read();
        if (done) break;
        if (value) this.onText(decoder.decode(value, { stream: true }));
      }
      this.onText(decoder.decode());
    } catch (error) {
      if (!this.stopping) failure = error;
    } finally {
      this.running = false;
      this.stopping = true;
      this.reader.releaseLock();
      this.reader = null;
      // All commands release their writer before port.close(), for both native
      // Web Serial and the Android CDC polyfill. close() cancels pending reads.
      await this.writeTask.catch(() => {});
      try { await this.port.close(); } catch (error) { failure ||= error; }
      this.port = null;
      this.onClose(failure);
    }
  }

  requestStatus() {
    this.writeTask = this.writeTask.catch(() => {}).then(async () => {
      if (!this.running || this.stopping) throw new Error("Die Konsole ist nicht verbunden.");
      const writer = this.port.writable.getWriter();
      try {
        // Read-only: connecting the console must not trigger an OTA update.
        await writer.write(new TextEncoder().encode("ota status\n"));
      } finally {
        writer.releaseLock();
      }
    });
    return this.writeTask;
  }

  async close() {
    this.stopping = true;
    try { await this.reader?.cancel(); } catch (_) {}
    await this.readTask;
  }
}
