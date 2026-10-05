# Web Serial polyfill

`web-serial-polyfill-1.0.15.mjs` is the unchanged `dist/serial.js` from Google's
[`web-serial-polyfill@1.0.15`](https://www.npmjs.com/package/web-serial-polyfill/v/1.0.15).
Only its filename has been changed to `.mjs`. Copyright and Apache-2.0 license
are retained; see `web-serial-polyfill-LICENSE.txt`.

- Upstream: https://github.com/google/web-serial-polyfill
- Published git revision: `a209091a2776aee2e0d5370e07cea8e521d16565`
- File SHA-256: `739e8a556a1453c87a619b2520dcbda39f0cea55c5d0bf865bc5967e7e4fcad0`
- The npm tarball was checked against the registry's SHA-512 integrity value.

The upstream repository is archived. This pinned copy implements USB CDC-ACM
for the CH9102 downloader (`1a86:55d4`); it is not a CP210x/CH340 driver.
`../serial-port.mjs` adapts `reconfigure()` to esptool-js's `setBaudRate()` hook
so changing baud does not close the device or toggle its reset lines.
