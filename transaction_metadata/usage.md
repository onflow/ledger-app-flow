## Generate merkle tree files from manifest files

Before starting the generation process you want to check if your manifest file does not contain new long transaction names and argument labels. If that is the case, then update `legerifyTxName`, `legerifyArgLabel` in `./tree/index.js`. If some of the new arguments is an enum argument built in in the app, you can update `enumToType`.

```
nvm use 16.10.0
cd tree
yarn run generate
cd ..
cd tests
yarn run generate
cd ..
```

This produces
- `txMerkleTree.js` to be used in JS client. 
- `txMerkleTree.mjs` used by the second yarn command. 
- `txMerkleTree.py` to be used in Python client.
- `testvectors/manifestPayloadCases.json`  to be used in manifest tests. Further files may be used for testing purposes in the future. They were used by hard to maintain unit tests in C++ before.

```
cp txMerkleTree.py ../tests/application_client/
cp testvectors/manifestPayloadCases.json ../tests/
cp txMerkleTree.js ../js/src
```

Depending on what you do with the JS sources, you may need to rebuild them.

Finally, you need to enter the root hash from `txMerkleTree.* files` (it should be the same in all files) to merkleTreeRoot variable `../src/from tx_metadata.c`

As the next step you need to fix speculos tests. 
You will need to generate new snapshots for the speculos tests.

You may probably need to fix some tests. Most notably `test_transaction_metadata_errors` and `test_transaction_slot`.
In the future we may jugde to add some fake transactions for testing into merkle tree to make the tests more stable.

Similarly, js tests may need to fix script hash and expected APDU sequence.


