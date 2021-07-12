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
 
const resolve = require("path").resolve;
 
export const APP_PATH = resolve("../app/bin/app.elf");
 
const APP_SEED = "equip will roof matter pink blind book anxiety banner elbow sun young"
 
export const simOptions = {
  logging: true,
  start_delay: 1500,
  custom: `-s "${APP_SEED}"`,
};
 
jest.setTimeout(100000)
