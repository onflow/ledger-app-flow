/** ******************************************************************************
 *  (c) 2020 Zondax GmbH
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ******************************************************************************* */

import jest from "jest";
import PNG from "pngjs";

const Resolve = require("path").resolve;

export const APP_PATH = Resolve("../app/bin/app.elf");

const APP_SEED = "equip will roof matter pink blind book anxiety banner elbow sun young"

export const simOptions = {
    //logging: true,
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

export async function prepareSlot(sim, app, slot, address = "0000000000000000", path = `m/0/0/0/0/0`) {
    const respRequest = app.setSlot(slot, address, path);
    await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());
    await sim.clickRight();
    await sim.clickRight();
    await sim.clickRight();
    await sim.clickBoth();
    return await respRequest;
}


