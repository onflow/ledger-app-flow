# @onflow/ledger

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![npm version](https://badge.fury.io/js/%40onflow%2Fledger.svg)](https://www.npmjs.com/package/@onflow/ledger)

This package provides a basic client library to communicate with the Flow App running in a Ledger Nano S/X

We recommend using the npmjs package in order to receive updates/fixes.

## Notes

Use `yarn install` to avoid issues.

## Minimal example

This is very simple. First you need to use one of the transport classes provided by Ledger.

```js
import TransportWebUSB from "@ledgerhq/hw-transport-webusb";
import FlowApp from "@onflow/ledger";

transport = await TransportWebUSB.create();
const app = new FlowApp(transport);
```

The `FlowApp` object will provide:

```js
const response = await app.getVersion();

if (response.returnCode !== FlowApp.ERROR_CODE.NoError) {
      console.log(`Error [${response.returnCode}] ${response.errorMessage}`);
} else {
    console.log(response);
}
```

## Tests running on ledger device

This folder's test folder contains tests to run on ledger device. The aim of the testing code is:
- To verify that the app works on the ledger device. 
- Testing in the early stages of feature development (this is, in some cases, easier done with ledger than with Zemu).
To attain this, we provide only some very basic tests (it does not make sense to have two parallel full scale integration tests). In development you should be able to easily modify the tests to suit your needs. 

To use the tests you first need to deploy the app to the ledger (with up to date firmware). As of now, you have to use non-containerized build (container's SDK is not up to date ATM). Then run `yarn test`. 

## Acknowledgements

Developed by [Zondax GmbH](https://zondax.ch/) and [Vacuumlabs](https://vacuumlabs.com/).
