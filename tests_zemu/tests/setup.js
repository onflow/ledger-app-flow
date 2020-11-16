import jest from "jest";
import PNG from "pngjs";

const Resolve = require("path").resolve;

export const APP_PATH = Resolve("../app/bin/app.elf");

const APP_SEED = "equip will roof matter pink blind book anxiety banner elbow sun young"

export const simOptions = {
    // logging: true,
    start_delay: 1500,
    custom: `-s "${APP_SEED}"`,
    // X11: true,
};

jest.setTimeout(60000)

function snapshotToPNG(rect) {
    const png = new PNG.PNG({
        width: rect.width,
        height: rect.height,
        data: rect.data,
    });

    png.data = rect.data.slice();

    const buffer = PNG.PNG.sync.write(png, { colorType: 6 });
    
    return buffer;
}

export async function verifyAndAccept(sim, pageCount) {
    // Wait until we are not in the main menu
    await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

    const snapshots = [];

    // Capture initial snapshot
    snapshots.push(snapshotToPNG(await sim.snapshot()));

    // Capture a snapshot of each transaction verification page
    for (let i = 0; i < pageCount; i++) {
        snapshots.push(snapshotToPNG(await sim.clickRight()));
    }

    // Finally, accept the transaction by clicking both buttons
    snapshots.push(snapshotToPNG(await sim.clickBoth()));

    return snapshots;
}
