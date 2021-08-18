import { toMatchImageSnapshot } from "jest-image-snapshot";

export default jest;
export const { expect, test } = global;

expect.extend({ toMatchImageSnapshot });
