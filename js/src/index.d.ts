import Transport from "@ledgerhq/hw-transport";

export interface ResponseBase {
  errorMessage: string;
  returnCode: number;
}

export interface ResponseAddress extends ResponseBase {
  publicKey: string;
  address: string;
}

export interface ResponseVersion extends ResponseBase {
  testMode: boolean;
  major: number;
  minor: number;
  patch: number;
  deviceLocked: boolean;
  targetId: string;
}

export interface ResponseAppInfo extends ResponseBase {
  appName: string;
  appVersion: string;
  flagLen: number;
  flagsValue: number;
  flagRecovery: boolean;
  flagSignedMcuCode: boolean;
  flagOnboarded: boolean;
  flagPINValidated: boolean;
}

export interface ResponseSign extends ResponseBase {
  signatureCompact: Buffer;
  signatureDER: Buffer;
}

export interface ResponseSlotStatus extends ResponseBase {
  status: Buffer;
}

export interface ResponseSlot extends ResponseBase {
  slotIdx: number;
  account: string;
  path: string;
}

export interface FlowApp {
  new(transport: Transport): FlowApp;

  getVersion(): Promise<ResponseVersion>;
  getAppInfo(): Promise<ResponseAppInfo>;
  getAddressAndPubKey(path: string): Promise<ResponseAddress>;
  showAddressAndPubKey(path: string): Promise<ResponseAddress>;

  slotStatus(): Promise<ResponseSlotStatus>;
  getSlot(slotIdx: number): Promise<ResponseSlot>;
  setSlot(slotIdx: number, account: string, path: string): Promise<ResponseSlot>;

  sign(path: string, message: Buffer): Promise<ResponseSign>;
}
