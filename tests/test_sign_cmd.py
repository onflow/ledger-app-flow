from pathlib import Path
import pytest
import re

from application_client.flow_command_sender import FlowCommandSender, Errors, HashType, ClaType, InsType, P1, CryptoOptions
from application_client.flow_response_unpacker import unpack_sign_tx_response
from application_client.txMerkleTree import merkleTree, merkleIndex

from ragger.bip import CurveChoice
from ragger.error import ExceptionRAPDU
from ragger.navigator import Navigator
from ragger.firmware import Firmware

import json

from utils import ROOT_SCREENSHOT_PATH, util_check_signature, util_check_pub_key, util_set_slot, util_set_expert_mode, util_navigate

MANIFEST_FILE = f"{ROOT_SCREENSHOT_PATH}/manifestPayloadCases.json"

# Note: Transactions are explained here: https://janezpodhostnik.github.io/flow-py-sdk/python_SDK_guide/#transactions


def _check_transaction(
        client: FlowCommandSender,
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path,
        transaction: str,
        path: str,
        crypto_options: CryptoOptions,
        signable_type: str,
        timeout: int = 300,
) -> None:
    """ Check the transaction in confirmation mode when user accepts """

    # Retrieve and Check the public key
    public_key = util_check_pub_key(client, path, crypto_options)

    # Convert message to bytes
    message = bytes.fromhex(transaction)

    # Send the APDU (Asynchronous)
    with client.sign_tx(path=path, crypto_options=crypto_options, transaction=message, hint=signable_type):
        util_navigate(firmware, navigator, test_name, "APPROVE_SIGN", timeout)

    # Send the APDU (Asynchronous)
    response = client.get_async_response()
    assert response.status == Errors.SW_SUCCESS

    # Parse the response
    _, der_sig = unpack_sign_tx_response(response.data)
    # Check the signature
    util_check_signature(public_key, der_sig, message, crypto_options, signable_type)

def get_tx_and_hash(titles, network):
    # Retrieve FA.01, FA.02, FA.03 from manifest
    with open(MANIFEST_FILE) as json_file:
        transactions = json.load(json_file)

    transactionsAndScriptHashes = []
    for transaction in transactions:
        tx_name = transaction["title"].split()[0]
        chain = transaction["chainID"]
        if tx_name in titles and chain == network: 
            transactionsAndScriptHashes.append((transaction["encodedTransactionEnvelopeHex"], transaction["hash"]))

    return transactionsAndScriptHashes

def test_transaction_metadata_errors(firmware, backend, navigator, test_name):
    """ Check metadata proofs. """

    # Use the app interface instead of raw interface
    client = FlowCommandSender(backend)

    def send_tx_body() -> bytes:
        dataToSend: List[bytes]= [
            bytes.fromhex("2c0000801b0200800102008000000000000000000103"),
            # This is FA.01 on Testnet, when the transaction changes, you need to modify the strings below according to the manifest
            bytes.fromhex("f90456b90300696d706f72742043727970746f0a0a7472616e73616374696f6e286b65793a20537472696e672c207369676e6174757265416c676f726974686d3a2055496e74382c2068617368416c676f726974686d3a2055496e74382c207765696768743a2055466978363429207b0a0970726570617265287369676e65723a206175746828426f72726f7756616c75652c2053746f726167652920264163636f756e7429207b0a0909707265207b0a0909097369676e6174757265416c676f726974686d203e3d2031202626207369676e6174757265416c676f726974686d203c3d20333a20224d7573742070726f766964652061207369676e617475"),
            bytes.fromhex("726520616c676f726974686d207261772076616c7565207468617420697320312c20322c206f722033220a09090968617368416c676f726974686d203e3d20312026262068617368416c676f726974686d203c3d20363a20224d7573742070726f766964652061206861736820616c676f726974686d207261772076616c75652074686174206973206265747765656e203120616e642036220a090909776569676874203c3d20313030302e303a2022546865206b657920776569676874206d757374206265206265747765656e203020616e642031303030220a09097d0a0a09096c6574207075626c69634b6579203d205075626c69634b6579280a0909"),
            bytes.fromhex("097075626c69634b65793a206b65792e6465636f646548657828292c0a0909097369676e6174757265416c676f726974686d3a205369676e6174757265416c676f726974686d2872617756616c75653a207369676e6174757265416c676f726974686d29210a0909290a0a09096c6574206163636f756e74203d204163636f756e742870617965723a207369676e6572290a0a09096163636f756e742e6b6579732e616464287075626c69634b65793a207075626c69634b65792c2068617368416c676f726974686d3a2048617368416c676f726974686d2872617756616c75653a2068617368416c676f726974686d29212c207765696768743a20776569"),
            bytes.fromhex("676874290a097d0a7df90110b8aa7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303665346634336637396433633164386361636233643566336537616565646232396665616562343535396664623731613937653266643034333835363533313065383736373030333564383362633130666536376665333134646261353336336338313635343539356436343838346231656361643135313261363465363565303230313634227d9c7b2276616c7565223a2231222c2274797065223a2255496e7438227d9c7b2276616c7565223a2231222c2274797065223a2255496e7438227da97b2276616c7565223a223130"),
            bytes.fromhex("30302e3030303030303030222c2274797065223a22554669783634227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a8899a8ac2c71d4f6bd040a8899a8ac2c71d4f6bdc98899a8ac2c71d4f6bd")
        ]

        backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_INIT, data=dataToSend[0])
        for i in range(1, len(dataToSend)):
            backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_ADD, data=dataToSend[i])

    #This is FA.01 on Testnet script hash. When the script changes, you need to update this according to the manifest
    scriptHash = "c4a7efd8708396e8c7a3611f72a9f89f675bf6d5c9336dd389e5839cba78443c"

    sI = merkleIndex[scriptHash[0:16]]
    correctMetadata: bytes = bytes.fromhex(merkleTree["children"][sI[0]]["children"][sI[1]]["children"][sI[2]]["children"][sI[3]]["children"][0])

    def get_proof_hex(merkleNode):
        return "".join(x["hash"] for x in merkleNode["children"])

    correctProof: List[bytes]= [
        bytes.fromhex(get_proof_hex(merkleTree["children"][sI[0]]["children"][sI[1]]["children"][sI[2]])),
        bytes.fromhex(get_proof_hex(merkleTree["children"][sI[0]]["children"][sI[1]])),
        bytes.fromhex(get_proof_hex(merkleTree["children"][sI[0]])),
        bytes.fromhex(get_proof_hex(merkleTree)),
    ]


    # Test metadata not matching the transaction 
    # We send metadata from the same node so that the proof is correct for otherMetadata
    assert sI[3] != 6 # if it is the case change [sI[3]+1] to [sI[3]-1] and change this assert to sI[3] != 0
    otherMetadata: bytes = bytes.fromhex(merkleTree["children"][sI[0]]["children"][sI[1]]["children"][sI[2]]["children"][sI[3]+1]["children"][0])
    # Proof in correctProof is correct
    send_tx_body()
    backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_METADATA, data=otherMetadata)
    backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_PROOF_ONGOING, data=correctProof[0])
    backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_PROOF_ONGOING, data=correctProof[1])
    backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_PROOF_ONGOING, data=correctProof[2])
    with pytest.raises(ExceptionRAPDU) as err:
        backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_PROOF_LAST, data=correctProof[3])
    assert err.value.status == Errors.SW_DATA_INVALID

    # Test error in first proof step
    wrongProofStep: bytes = bytes.fromhex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")
    send_tx_body()
    with pytest.raises(ExceptionRAPDU) as err:
        backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_METADATA, data=correctMetadata)
        backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_PROOF_ONGOING, data=wrongProofStep)
    assert err.value.status == Errors.SW_DATA_INVALID

    # Test error in second proof
    send_tx_body()
    backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_METADATA, data=correctMetadata)
    backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_PROOF_ONGOING, data=correctProof[0])
    with pytest.raises(ExceptionRAPDU) as err2:
        backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_PROOF_ONGOING, data=wrongProofStep)
    assert err2.value.status == Errors.SW_DATA_INVALID

    # Error in the last step
    send_tx_body()
    backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_METADATA, data=correctMetadata)
    backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_PROOF_ONGOING, data=correctProof[0])
    backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_PROOF_ONGOING, data=correctProof[1])
    backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_PROOF_ONGOING, data=correctProof[2])
    with pytest.raises(ExceptionRAPDU) as err:
        backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_PROOF_LAST, data=wrongProofStep)
    assert err.value.status == Errors.SW_DATA_INVALID

    # Error comparing final hashes, last byte different
    wrongLastProofStep: bytes = correctProof[3][0:-1]+bytes.fromhex("00")
    send_tx_body()
    backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_METADATA, data=correctMetadata)
    backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_PROOF_ONGOING, data=correctProof[0])
    backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_PROOF_ONGOING, data=correctProof[1])
    backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_PROOF_ONGOING, data=correctProof[2])
    with pytest.raises(ExceptionRAPDU) as err:
        backend.exchange(cla=ClaType.CLA_APP, ins=InsType.SIGN, p1=P1.P1_PROOF_LAST, data=wrongLastProofStep)
    assert err.value.status == Errors.SW_DATA_INVALID


def test_transaction_params(firmware, backend, navigator, test_name):
    """ Check transaction signing with different parameters.
     Uses FA.01, FA.02, FA.03 mainnet transactions from manifest."""

    # Use the app interface instead of raw interface
    client = FlowCommandSender(backend)

    # Retrieve FA.01, FA.02, FA.03 from manifest
    transactionsAndScriptHashes = get_tx_and_hash(("FA.01", "FA.02", "FA.03"), "Mainnet")

    # Test parameters
    path: str = "m/44'/539'/513'/0/0"
    curve_list = [
        CurveChoice.Secp256k1,
        CurveChoice.Nist256p1,
    ]
    hash_list = [
        HashType.HASH_SHA2,
        HashType.HASH_SHA3,
    ]

    # Send the APDU and check the results
    part = 0
    for transaction in transactionsAndScriptHashes:
        for curve in curve_list:
            for hash_t in hash_list:
                part += 1
                _check_transaction(client, firmware, navigator, f"{test_name}/part{part}", transaction[0], path, CryptoOptions(curve, hash_t), transaction[1])


class Test_EXPERT():
    def test_transaction_expert(self, firmware, backend, navigator, test_name):
        """ Check transaction signing with expert mode.
         Uses FA.03 mainnet transaction """

        # Use the app interface instead of raw interface
        client = FlowCommandSender(backend)

        # Test parameters
        path: str = "m/44'/539'/0'/0/0"
        test_cfg = [
            CryptoOptions(CurveChoice.Secp256k1, HashType.HASH_SHA2),
            CryptoOptions(CurveChoice.Nist256p1, HashType.HASH_SHA3),
        ]

        # Retrieve FA.03 from manifest
        transactionsAndScriptHashes = get_tx_and_hash(("FA.03",), "Mainnet")

        transaction = transactionsAndScriptHashes[0][0]
        txHash = transactionsAndScriptHashes[0][1]

        part = 0
        # Navigate in the main menu to change to expert mode
        util_set_expert_mode(firmware, navigator, f"{test_name}/part{part}")

        # Send the APDU and check the results
        for cfg in test_cfg:
            part += 1
            _check_transaction(client, firmware, navigator, f"{test_name}/part{part}", transaction, path, cfg, txHash)


def test_transaction_slot(firmware, backend, navigator, test_name):
    """ Check transaction signing with slot """

    # Use the app interface instead of raw interface
    client = FlowCommandSender(backend)
    # Test parameters
    path: str = "m/44'/539'/771'/0/0"
    bad_path: str = "m/44'/539'/771'/0/1"
    curve: CurveChoice = CurveChoice.Nist256p1
    crypto_options: CryptoOptions = CryptoOptions(curve, HashType.HASH_SHA3)
    bad_hash: HashType = HashType.HASH_SHA2
    slot = 0
    address = "f8d6e0586b0a20c7"

    # Retrieve FA.02 from manifest
    transactionsAndScriptHashes = get_tx_and_hash(("FA.02",), "Mainnet")
    transaction = transactionsAndScriptHashes[0][0]
    scriptHash = transactionsAndScriptHashes[0][1]

    # Send the APDU and check the results
    part = 0
    _check_transaction(client, firmware, navigator, f"{test_name}/part{part}", transaction, path, crypto_options, scriptHash)

    # Set slot to correct path correct address,
    part += 1
    util_set_slot(client, firmware, navigator, f"{test_name}/part{part}", slot, crypto_options, address, path)

    # Sign the Tx again - incorrect hd path
    part += 1
    _check_transaction(client, firmware, navigator, f"{test_name}/part{part}", transaction, bad_path, crypto_options, scriptHash)

    # Sign the Tx again - correct path
    part += 1
    _check_transaction(client, firmware, navigator, f"{test_name}/part{part}", transaction, path, crypto_options, scriptHash)

    # f19c161bc24cf4b4 - used as incorrect address; f8d6e0586b0a20c7 - correct one
    print(transaction)
    ap =  [m.start() for m in re.finditer("f19c161bc24cf4b4", transaction)]
    assert len(ap) == 3

    # tx - no match single authorizer,
    tx1 = transaction[:ap[0]]+"f19c161bc24cf4b4"+transaction[(ap[0]+16):ap[1]]+"f19c161bc24cf4b4"+transaction[(ap[1]+16):ap[2]]+"f19c161bc24cf4b4"+transaction[(ap[2]+16):]
    # tx - address matches payer
    tx2 = transaction[:ap[0]]+"f19c161bc24cf4b4"+transaction[(ap[0]+16):ap[1]]+"f8d6e0586b0a20c7"+transaction[(ap[1]+16):ap[2]]+"f19c161bc24cf4b4"+transaction[(ap[2]+16):]
    # tx - address matches proposer
    tx3 = transaction[:ap[0]]+"f8d6e0586b0a20c7"+transaction[(ap[0]+16):ap[1]]+"f19c161bc24cf4b4"+transaction[(ap[1]+16):ap[2]]+"f19c161bc24cf4b4"+transaction[(ap[2]+16):]
    # tx - address matches sole authorizer
    tx4 = transaction[:ap[0]]+"f19c161bc24cf4b4"+transaction[(ap[0]+16):ap[1]]+"f19c161bc24cf4b4"+transaction[(ap[1]+16):ap[2]]+"f8d6e0586b0a20c7"+transaction[(ap[2]+16):]

    # use online rlp encoder/decoder to fix these when FA.02 changes
    # decode these transactions and he new transactions and hopefully you will know what to change and encode this
    # multiple authorisers, no match
    tx5 = "f90435f90431b902c9696d706f72742043727970746f0a0a7472616e73616374696f6e286b65793a20537472696e672c207369676e6174757265416c676f726974686d3a2055496e74382c2068617368416c676f726974686d3a2055496e74382c207765696768743a2055466978363429207b0a0a0970726570617265287369676e65723a2061757468284164644b65792920264163636f756e7429207b0a0909707265207b0a0909097369676e6174757265416c676f726974686d203e3d2031202626207369676e6174757265416c676f726974686d203c3d20333a20224d7573742070726f766964652061207369676e617475726520616c676f726974686d207261772076616c7565207468617420697320312c20322c206f722033220a09090968617368416c676f726974686d203e3d20312026262068617368416c676f726974686d203c3d20363a20224d7573742070726f766964652061206861736820616c676f726974686d207261772076616c75652074686174206973206265747765656e203120616e642036220a090909776569676874203c3d20313030302e303a2022546865206b657920776569676874206d757374206265206265747765656e203020616e642031303030220a09097d0a09096c6574207075626c69634b6579203d205075626c69634b6579280a0909097075626c69634b65793a206b65792e6465636f646548657828292c0a0909097369676e6174757265416c676f726974686d3a205369676e6174757265416c676f726974686d2872617756616c75653a207369676e6174757265416c676f726974686d29210a0909290a0a09097369676e65722e6b6579732e616464287075626c69634b65793a207075626c69634b65792c2068617368416c676f726974686d3a2048617368416c676f726974686d2872617756616c75653a2068617368416c676f726974686d29212c207765696768743a20776569676874290a097d0a7df90110b8aa7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303665346634336637396433633164386361636233643566336537616565646232396665616562343535396664623731613937653266643034333835363533313065383736373030333564383362633130666536376665333134646261353336336338313635343539356436343838346231656361643135313261363465363565303230313634227d9c7b2276616c7565223a2231222c2274797065223a2255496e7438227d9c7b2276616c7565223a2231222c2274797065223a2255496e7438227da97b2276616c7565223a22313030302e3030303030303030222c2274797065223a22554669783634227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f19c161bc24cf4b4040a88f19c161bc24cf4b4db88f19c161bc24cf4b488f19c161bc24cf4b488f19c161bc24cf4b4c0"
    # tx - address matches 3rd auhorizer out of three
    tx6 = "f90435f90431b902c9696d706f72742043727970746f0a0a7472616e73616374696f6e286b65793a20537472696e672c207369676e6174757265416c676f726974686d3a2055496e74382c2068617368416c676f726974686d3a2055496e74382c207765696768743a2055466978363429207b0a0a0970726570617265287369676e65723a2061757468284164644b65792920264163636f756e7429207b0a0909707265207b0a0909097369676e6174757265416c676f726974686d203e3d2031202626207369676e6174757265416c676f726974686d203c3d20333a20224d7573742070726f766964652061207369676e617475726520616c676f726974686d207261772076616c7565207468617420697320312c20322c206f722033220a09090968617368416c676f726974686d203e3d20312026262068617368416c676f726974686d203c3d20363a20224d7573742070726f766964652061206861736820616c676f726974686d207261772076616c75652074686174206973206265747765656e203120616e642036220a090909776569676874203c3d20313030302e303a2022546865206b657920776569676874206d757374206265206265747765656e203020616e642031303030220a09097d0a09096c6574207075626c69634b6579203d205075626c69634b6579280a0909097075626c69634b65793a206b65792e6465636f646548657828292c0a0909097369676e6174757265416c676f726974686d3a205369676e6174757265416c676f726974686d2872617756616c75653a207369676e6174757265416c676f726974686d29210a0909290a0a09097369676e65722e6b6579732e616464287075626c69634b65793a207075626c69634b65792c2068617368416c676f726974686d3a2048617368416c676f726974686d2872617756616c75653a2068617368416c676f726974686d29212c207765696768743a20776569676874290a097d0a7df90110b8aa7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303665346634336637396433633164386361636233643566336537616565646232396665616562343535396664623731613937653266643034333835363533313065383736373030333564383362633130666536376665333134646261353336336338313635343539356436343838346231656361643135313261363465363565303230313634227d9c7b2276616c7565223a2231222c2274797065223a2255496e7438227d9c7b2276616c7565223a2231222c2274797065223a2255496e7438227da97b2276616c7565223a22313030302e3030303030303030222c2274797065223a22554669783634227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f19c161bc24cf4b4040a88f19c161bc24cf4b4db88f19c161bc24cf4b488f19c161bc24cf4b488f8d6e0586b0a20c7c0"

    transactions = [tx1, tx2, tx3, tx4, tx5, tx6]

    # Send the APDU and check the results
    for blob in transactions:
        part += 1
        _check_transaction(client, firmware, navigator, f"{test_name}/part{part}", blob, path, crypto_options, scriptHash)

    # sign the Tx again - correct path - wrong hash
        part += 1
    _check_transaction(client, firmware, navigator, f"{test_name}/part{part}", transaction, path, CryptoOptions(curve, bad_hash), scriptHash)

    # Now delete the slot so that the next test starts in a clean state
    util_set_slot(client, firmware, navigator, test_name, slot)


def test_transaction_invalid(firmware, backend, navigator, test_name):
    """ Check invalid transaction signing """

    # Use the app interface instead of raw interface
    client = FlowCommandSender(backend)
    # Test parameters
    path: str = "m/44'/539'/513'/0/0"
    crypto_options: CryptoOptions = CryptoOptions(CurveChoice.Secp256k1, HashType.HASH_SHA2)
    # Prepare an invalid message
    bad_message = "1234567890"
    transaction = bad_message.encode("utf-8").hex()

    # Send the APDU and check the results
    try:
        _check_transaction(client, firmware, navigator, test_name, transaction, path, crypto_options, "", 5)
    except TimeoutError:
        pass


def test_transaction_refused(firmware, backend, navigator, test_name):
    """ Check transaction signing in confirmation mode when user refuses """

    # Use the app interface instead of raw interface
    client = FlowCommandSender(backend)
    # Test parameters
    path: str = "m/44'/539'/0'/0/0"
    crypto_options: CryptoOptions = CryptoOptions(CurveChoice.Nist256p1, HashType.HASH_SHA2)

    # Retrieve FA.01 from manifest
    transactionsAndScriptHashes = get_tx_and_hash(("FA.01",), "Mainnet")
    transaction = transactionsAndScriptHashes[0][0]
    scriptHash = transactionsAndScriptHashes[0][1]

    # Convert message to bytes
    message = bytes.fromhex(transaction)

    # Send the APDU (Asynchronous)
    with pytest.raises(ExceptionRAPDU) as err:
        with client.sign_tx(path=path, crypto_options=crypto_options, transaction=message, hint=scriptHash):
            util_navigate(firmware, navigator, test_name, "REJECT_SIGN")

    # Assert we have received a refusal
    assert err.value.status == Errors.SW_COMMAND_NOT_ALLOWED
    assert len(err.value.data) == 0


def test_transaction_manifest(firmware, backend, navigator, test_name):
    """ Check transaction based on manifest file """

    # Use the app interface instead of raw interface
    client = FlowCommandSender(backend)
    # Test parameters
    path: str = "m/44'/539'/0'/0/0"
    crypto_options: CryptoOptions = CryptoOptions(CurveChoice.Nist256p1, HashType.HASH_SHA3)

    with open(MANIFEST_FILE) as json_file:
        transactions = json.load(json_file)
    
    # Send the APDU and check the results
    for transaction in transactions:
        title_split = transaction["title"].split()
        tx_name = transaction["title"].split()[0]
        if len(title_split) > 3 and title_split[-2] == "-":
            tx_name = tx_name+"-"+title_split[-1]
        chain = transaction["chainID"]
        _check_transaction(client, firmware, navigator, f"{test_name}/{tx_name}-{chain}", transaction["encodedTransactionEnvelopeHex"], path, crypto_options, transaction["hash"])
        
class Test_MESSAGE():
    def test_message_normal(self, firmware, backend, navigator, test_name):
        """ Check message signing, short message """

        # Use the app interface instead of raw interface
        client = FlowCommandSender(backend)
        # Test parameters
        path: str = "m/44'/539'/0'/0/0"
        test_cfg = [
            {
                "options": CryptoOptions(CurveChoice.Secp256k1, HashType.HASH_SHA2),
                # "This is a nice message that has only displayable characters and is short enough to be displayed"
                "message": "546869732069732061206e696365206d657373616765207468617420686173206f6e6c7920646973706c617961626c65206368617261637465727320616e642069732073686f727420656e6f75676820746f20626520646973706c61796564"
            },
            {
                "options": CryptoOptions(CurveChoice.Secp256k1, HashType.HASH_SHA2),
                # Message too long to be displayed normally
                "message": 1000*"40"
            },
            {
                "options": CryptoOptions(CurveChoice.Nist256p1, HashType.HASH_SHA3),
                # "This is a nice message that has only displayable characters and is short enough to be displayed"
                "message": "546869732069732061206e696365206d657373616765207468617420686173206f6e6c7920646973706c617961626c65206368617261637465727320616e642069732073686f727420656e6f75676820746f20626520646973706c61796564"
            },
            {
                "options": CryptoOptions(CurveChoice.Secp256k1, HashType.HASH_SHA2),
                # Message too long to be displayed normally
                "message": 1000*"40"
            },
        ]
        
        part = 0

        # Send the APDU and check the results
        for i,cfg in enumerate(test_cfg):
            _check_transaction(client, firmware, navigator, f"{test_name}/part{part}", cfg["message"], path, cfg["options"], "message")
            part += 1
            if i == 0 or i == 3:
                # Navigate in the main menu to change to expert mode          
                util_set_expert_mode(firmware, navigator, f"{test_name}/part{part}")
                part += 1


    def test_message_invalid(self, firmware, backend, navigator, test_name):
        """ Check message signing, message with non-displayale character """

        # Use the app interface instead of raw interface
        client = FlowCommandSender(backend)
        # Test parameters
        path: str = "m/44'/539'/0'/0/0"
        cryptoOptions = CryptoOptions(CurveChoice.Secp256k1, HashType.HASH_SHA2)
        test_cfg = [
            {
                "options": cryptoOptions,
                # Message with non-displayable characters
                "message": "4d657373616765ee"
            },
            {
                "options": cryptoOptions,
                # Message too long to display and expert mode is off
                "message": 1000*"40"
            },
        ]
        
        part = 0

        # Send the APDU and check the results
        for cfg in test_cfg:
            # Convert message to bytes
            message = bytes.fromhex(cfg["message"])

            # Send the APDU (Asynchronous)
            with pytest.raises(ExceptionRAPDU) as err:
                with client.sign_tx(path, cryptoOptions , message, "message"):
                    pass
            assert(str(err) == "<ExceptionInfo ExceptionRAPDU(status=27012, data=b'Invalid message') tblen=8>")
            part += 1


class Test_ARBITRARY():
    def test_arbitrary_transaction_signing_fail_in_no_expert_mode(self, firmware, backend, navigator, test_name):
        """ Check arbitrary transaction signing without expert mode - should fail """

        # Use the app interface instead of raw interface
        client = FlowCommandSender(backend)
        # Test parameters
        path: str = "m/44'/539'/0'/0/0"
        cryptoOptions = CryptoOptions(CurveChoice.Secp256k1, HashType.HASH_SHA2)

        cfg = {
	        "tx": "f906e9f906e5b90423696d706f727420466c6f775374616b696e67436f6c6c656374696f6e2066726f6d203078386430653837623635313539616536330a0a2f2f2f20437265617465732061206d616368696e65206163636f756e7420666f722061206e6f6465207468617420697320616c726561647920696e20746865207374616b696e6720636f6c6c656374696f6e0a2f2f2f20616e642061646473207075626c6963206b65797320746f20746865206e6577206163636f756e740a0a7472616e73616374696f6e286e6f646549443a20537472696e672c207075626c69634b6579733a205b537472696e675d29207b0a202020200a202020206c6574207374616b696e67436f6c6c656374696f6e5265663a2026466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e0a0a2020202070726570617265286163636f756e743a20417574684163636f756e7429207b0a202020202020202073656c662e7374616b696e67436f6c6c656374696f6e526566203d206163636f756e742e626f72726f773c26466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e3e2866726f6d3a20466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e53746f7261676550617468290a2020202020202020202020203f3f2070616e69632822436f756c64206e6f7420626f72726f772072656620746f205374616b696e67436f6c6c656374696f6e22290a0a20202020202020206966206c6574206d616368696e654163636f756e74203d2073656c662e7374616b696e67436f6c6c656374696f6e5265662e6372656174654d616368696e654163636f756e74466f724578697374696e674e6f6465286e6f646549443a206e6f646549442c2070617965723a206163636f756e7429207b0a2020202020202020202020206966207075626c69634b657973203d3d206e696c207c7c207075626c69634b657973212e6c656e677468203d3d2030207b0a2020202020202020202020202020202070616e6963282243616e6e6f742070726f76696465207a65726f206b65797320666f7220746865206d616368696e65206163636f756e7422290a2020202020202020202020207d0a202020202020202020202020666f72206b657920696e207075626c69634b65797321207b0a202020202020202020202020202020206d616368696e654163636f756e742e6164645075626c69634b6579286b65792e6465636f64654865782829290a2020202020202020202020207d0a20202020202020207d20656c7365207b0a20202020202020202020202070616e69632822436f756c64206e6f74206372656174652061206d616368696e65206163636f756e7420666f7220746865206e6f646522290a20202020202020207d0a202020207d0a7d0af9027cb85c7b2274797065223a22537472696e67222c2276616c7565223a2238383534393333356531646237623562343663326164353864646237306237613435653737306363356665373739363530626132366631306536626165356536227db9021b7b2274797065223a224172726179222c2276616c7565223a5b7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303665346634336637396433633164386361636233643566336537616565646232396665616562343535396664623731613937653266643034333835363533313065383736373030333564383362633130666536376665333134646261353336336338313635343539356436343838346231656361643135313261363465363565303230313634227d2c7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303665346634336637396433633164386361636233643566336537616565646232396665616562343535396664623731613937653266643034333835363533313065383736373030333564383362633130666536376665333134646261353336336338313635343539356436343838346231656361643135313261363465363565303230313634227d2c7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303665346634336637396433633164386361636233643566336537616565646232396665616562343535396664623731613937653266643034333835363533313065383736373030333564383362633130666536376665333134646261353336336338313635343539356436343838346231656361643135313261363465363565303230313634227d5d7da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f19c161bc24cf4b4040a88f19c161bc24cf4b4c988f19c161bc24cf4b4c0",
                "options": cryptoOptions,
        }
        
        part = 0
        # Send the APDU and check the results
        with pytest.raises(ExceptionRAPDU) as err:
            with client.sign_tx(path, cryptoOptions, bytes.fromhex(cfg["tx"]), "arbitrary"):
                pass
        assert(str(err) == "<ExceptionInfo ExceptionRAPDU(status=27012, data=b'Unexpected script') tblen=8>")

    def test_arbitrary_transaction_signing_expert(self, firmware, backend, navigator, test_name):
        """ Check arbitrary transaction signing with expert mode """

        # Use the app interface instead of raw interface
        client = FlowCommandSender(backend)
        # Test parameters
        path: str = "m/44'/539'/0'/0/0"
        test_cfg = [
            {
	        "tx": "f906e9f906e5b90423696d706f727420466c6f775374616b696e67436f6c6c656374696f6e2066726f6d203078386430653837623635313539616536330a0a2f2f2f20437265617465732061206d616368696e65206163636f756e7420666f722061206e6f6465207468617420697320616c726561647920696e20746865207374616b696e6720636f6c6c656374696f6e0a2f2f2f20616e642061646473207075626c6963206b65797320746f20746865206e6577206163636f756e740a0a7472616e73616374696f6e286e6f646549443a20537472696e672c207075626c69634b6579733a205b537472696e675d29207b0a202020200a202020206c6574207374616b696e67436f6c6c656374696f6e5265663a2026466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e0a0a2020202070726570617265286163636f756e743a20417574684163636f756e7429207b0a202020202020202073656c662e7374616b696e67436f6c6c656374696f6e526566203d206163636f756e742e626f72726f773c26466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e3e2866726f6d3a20466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e53746f7261676550617468290a2020202020202020202020203f3f2070616e69632822436f756c64206e6f7420626f72726f772072656620746f205374616b696e67436f6c6c656374696f6e22290a0a20202020202020206966206c6574206d616368696e654163636f756e74203d2073656c662e7374616b696e67436f6c6c656374696f6e5265662e6372656174654d616368696e654163636f756e74466f724578697374696e674e6f6465286e6f646549443a206e6f646549442c2070617965723a206163636f756e7429207b0a2020202020202020202020206966207075626c69634b657973203d3d206e696c207c7c207075626c69634b657973212e6c656e677468203d3d2030207b0a2020202020202020202020202020202070616e6963282243616e6e6f742070726f76696465207a65726f206b65797320666f7220746865206d616368696e65206163636f756e7422290a2020202020202020202020207d0a202020202020202020202020666f72206b657920696e207075626c69634b65797321207b0a202020202020202020202020202020206d616368696e654163636f756e742e6164645075626c69634b6579286b65792e6465636f64654865782829290a2020202020202020202020207d0a20202020202020207d20656c7365207b0a20202020202020202020202070616e69632822436f756c64206e6f74206372656174652061206d616368696e65206163636f756e7420666f7220746865206e6f646522290a20202020202020207d0a202020207d0a7d0af9027cb85c7b2274797065223a22537472696e67222c2276616c7565223a2238383534393333356531646237623562343663326164353864646237306237613435653737306363356665373739363530626132366631306536626165356536227db9021b7b2274797065223a224172726179222c2276616c7565223a5b7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303665346634336637396433633164386361636233643566336537616565646232396665616562343535396664623731613937653266643034333835363533313065383736373030333564383362633130666536376665333134646261353336336338313635343539356436343838346231656361643135313261363465363565303230313634227d2c7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303665346634336637396433633164386361636233643566336537616565646232396665616562343535396664623731613937653266643034333835363533313065383736373030333564383362633130666536376665333134646261353336336338313635343539356436343838346231656361643135313261363465363565303230313634227d2c7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303665346634336637396433633164386361636233643566336537616565646232396665616562343535396664623731613937653266643034333835363533313065383736373030333564383362633130666536376665333134646261353336336338313635343539356436343838346231656361643135313261363465363565303230313634227d5d7da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f19c161bc24cf4b4040a88f19c161bc24cf4b4c988f19c161bc24cf4b4c0",
                "options": CryptoOptions(CurveChoice.Secp256k1, HashType.HASH_SHA2),
            },
            {
	        "tx": "f904e6f904e2b90423696d706f727420466c6f775374616b696e67436f6c6c656374696f6e2066726f6d203078393565303139613137643065323364370a0a2f2f2f20437265617465732061206d616368696e65206163636f756e7420666f722061206e6f6465207468617420697320616c726561647920696e20746865207374616b696e6720636f6c6c656374696f6e0a2f2f2f20616e642061646473207075626c6963206b65797320746f20746865206e6577206163636f756e740a0a7472616e73616374696f6e286e6f646549443a20537472696e672c207075626c69634b6579733a205b537472696e675d29207b0a202020200a202020206c6574207374616b696e67436f6c6c656374696f6e5265663a2026466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e0a0a2020202070726570617265286163636f756e743a20417574684163636f756e7429207b0a202020202020202073656c662e7374616b696e67436f6c6c656374696f6e526566203d206163636f756e742e626f72726f773c26466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e3e2866726f6d3a20466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e53746f7261676550617468290a2020202020202020202020203f3f2070616e69632822436f756c64206e6f7420626f72726f772072656620746f205374616b696e67436f6c6c656374696f6e22290a0a20202020202020206966206c6574206d616368696e654163636f756e74203d2073656c662e7374616b696e67436f6c6c656374696f6e5265662e6372656174654d616368696e654163636f756e74466f724578697374696e674e6f6465286e6f646549443a206e6f646549442c2070617965723a206163636f756e7429207b0a2020202020202020202020206966207075626c69634b657973203d3d206e696c207c7c207075626c69634b657973212e6c656e677468203d3d2030207b0a2020202020202020202020202020202070616e6963282243616e6e6f742070726f76696465207a65726f206b65797320666f7220746865206d616368696e65206163636f756e7422290a2020202020202020202020207d0a202020202020202020202020666f72206b657920696e207075626c69634b65797321207b0a202020202020202020202020202020206d616368696e654163636f756e742e6164645075626c69634b6579286b65792e6465636f64654865782829290a2020202020202020202020207d0a20202020202020207d20656c7365207b0a20202020202020202020202070616e69632822436f756c64206e6f74206372656174652061206d616368696e65206163636f756e7420666f7220746865206e6f646522290a20202020202020207d0a202020207d0a7d0af87ab85c7b2274797065223a22537472696e67222c2276616c7565223a2238383534393333356531646237623562343663326164353864646237306237613435653737306363356665373739363530626132366631306536626165356536227d9b7b2274797065223a224172726179222c2276616c7565223a5b5d7da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a8899a8ac2c71d4f6bd040a8899a8ac2c71d4f6bdc98899a8ac2c71d4f6bdc0",
                "options": CryptoOptions(CurveChoice.Nist256p1, HashType.HASH_SHA3),
            },
            {
	        "tx": "f9039df90399b902a4696d706f727420466c6f775374616b696e67436f6c6c656374696f6e2066726f6d203078393565303139613137643065323364370a0a2f2f2f20526571756573747320756e7374616b696e6720666f722074686520737065636966696564206e6f6465206f722064656c656761746f7220696e20746865207374616b696e6720636f6c6c656374696f6e0a0a7472616e73616374696f6e286e6f646549443a20537472696e672c2064656c656761746f7249443a2055496e7433323f2c20616d6f756e743a2055466978363429207b0a202020200a202020206c6574207374616b696e67436f6c6c656374696f6e5265663a2026466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e0a0a2020202070726570617265286163636f756e743a20417574684163636f756e7429207b0a202020202020202073656c662e7374616b696e67436f6c6c656374696f6e526566203d206163636f756e742e626f72726f773c26466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e3e2866726f6d3a20466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e53746f7261676550617468290a2020202020202020202020203f3f2070616e69632822436f756c64206e6f7420626f72726f772072656620746f205374616b696e67436f6c6c656374696f6e22290a202020207d0a0a2020202065786563757465207b0a202020202020202073656c662e7374616b696e67436f6c6c656374696f6e5265662e72657175657374556e7374616b696e67286e6f646549443a206e6f646549442c2064656c656761746f7249443a2064656c656761746f7249442c20616d6f756e743a20616d6f756e74290a202020207d0a7d0af8b0b85c7b2274797065223a22537472696e67222c2276616c7565223a2238383534393333356531646237623562343663326164353864646237306237613435653737306363356665373739363530626132366631306536626165356536227da07b2274797065223a224f7074696f6e616c222c2276616c7565223a6e756c6c7db07b2274797065223a22554669783634222c2276616c7565223a2239323233333732303336382e3534373735383038227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a8899a8ac2c71d4f6bd040a8899a8ac2c71d4f6bdc98899a8ac2c71d4f6bdc0",
                "options": CryptoOptions(CurveChoice.Secp256k1, HashType.HASH_SHA2),
            },
            {
	        "tx": "f903b8f903b4b902a4696d706f727420466c6f775374616b696e67436f6c6c656374696f6e2066726f6d203078386430653837623635313539616536330a0a2f2f2f20526571756573747320756e7374616b696e6720666f722074686520737065636966696564206e6f6465206f722064656c656761746f7220696e20746865207374616b696e6720636f6c6c656374696f6e0a0a7472616e73616374696f6e286e6f646549443a20537472696e672c2064656c656761746f7249443a2055496e7433323f2c20616d6f756e743a2055466978363429207b0a202020200a202020206c6574207374616b696e67436f6c6c656374696f6e5265663a2026466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e0a0a2020202070726570617265286163636f756e743a20417574684163636f756e7429207b0a202020202020202073656c662e7374616b696e67436f6c6c656374696f6e526566203d206163636f756e742e626f72726f773c26466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e3e2866726f6d3a20466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e53746f7261676550617468290a2020202020202020202020203f3f2070616e69632822436f756c64206e6f7420626f72726f772072656620746f205374616b696e67436f6c6c656374696f6e22290a202020207d0a0a2020202065786563757465207b0a202020202020202073656c662e7374616b696e67436f6c6c656374696f6e5265662e72657175657374556e7374616b696e67286e6f646549443a206e6f646549442c2064656c656761746f7249443a2064656c656761746f7249442c20616d6f756e743a20616d6f756e74290a202020207d0a7d0af8cbb85c7b2274797065223a22537472696e67222c2276616c7565223a2238383534393333356531646237623562343663326164353864646237306237613435653737306363356665373739363530626132366631306536626165356536227db83a7b2274797065223a224f7074696f6e616c222c2276616c7565223a7b2274797065223a2255496e743332222c2276616c7565223a223432227d7db07b2274797065223a22554669783634222c2276616c7565223a2239323233333732303336382e3534373735383038227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f19c161bc24cf4b4040a88f19c161bc24cf4b4c988f19c161bc24cf4b4c0",
                "options": CryptoOptions(CurveChoice.Secp256k1, HashType.HASH_SHA2),
            },
            {
	        "tx": "f90864f90860b9058a696d706f72742043727970746f0a696d706f727420466c6f775374616b696e67436f6c6c656374696f6e2066726f6d203078393565303139613137643065323364370a0a2f2f2f2052656769737465727320612064656c656761746f7220696e20746865207374616b696e6720636f6c6c656374696f6e207265736f757263650a2f2f2f20666f722074686520737065636966696564206e6f646520696e666f726d6174696f6e20616e642074686520616d6f756e74206f6620746f6b656e7320746f20636f6d6d69740a0a7472616e73616374696f6e2869643a20537472696e672c0a202020202020202020202020726f6c653a2055496e74382c0a2020202020202020202020206e6574776f726b696e67416464726573733a20537472696e672c0a2020202020202020202020206e6574776f726b696e674b65793a20537472696e672c0a2020202020202020202020207374616b696e674b65793a20537472696e672c0a202020202020202020202020616d6f756e743a205546697836342c0a2020202020202020202020207075626c69634b6579733a205b43727970746f2e4b65794c697374456e7472795d3f29207b0a202020200a202020206c6574207374616b696e67436f6c6c656374696f6e5265663a2026466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e0a0a2020202070726570617265286163636f756e743a20417574684163636f756e7429207b0a202020202020202073656c662e7374616b696e67436f6c6c656374696f6e526566203d206163636f756e742e626f72726f773c26466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e3e2866726f6d3a20466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e53746f7261676550617468290a2020202020202020202020203f3f2070616e69632822436f756c64206e6f7420626f72726f772072656620746f205374616b696e67436f6c6c656374696f6e22290a0a20202020202020206966206c6574206d616368696e654163636f756e74203d2073656c662e7374616b696e67436f6c6c656374696f6e5265662e72656769737465724e6f6465280a20202020202020202020202069643a2069642c0a202020202020202020202020726f6c653a20726f6c652c0a2020202020202020202020206e6574776f726b696e67416464726573733a206e6574776f726b696e67416464726573732c0a2020202020202020202020206e6574776f726b696e674b65793a206e6574776f726b696e674b65792c0a2020202020202020202020207374616b696e674b65793a207374616b696e674b65792c0a202020202020202020202020616d6f756e743a20616d6f756e742c0a20202020202020202020202070617965723a206163636f756e7429200a20202020202020207b0a2020202020202020202020206966207075626c69634b657973203d3d206e696c207c7c207075626c69634b657973212e6c656e677468203d3d2030207b0a2020202020202020202020202020202070616e6963282243616e6e6f742070726f76696465207a65726f206b65797320666f7220746865206d616368696e65206163636f756e7422290a2020202020202020202020207d0a202020202020202020202020666f72206b657920696e207075626c69634b65797321207b0a202020202020202020202020202020206d616368696e654163636f756e742e6b6579732e616464287075626c69634b65793a206b65792e7075626c69634b65792c2068617368416c676f726974686d3a206b65792e68617368416c676f726974686d2c207765696768743a206b65792e776569676874290a2020202020202020202020207d0a20202020202020207d0a202020207d0a7d0af90290b85c7b2274797065223a22537472696e67222c2276616c7565223a2238383534393333356531646237623562343663326164353864646237306237613435653737306363356665373739363530626132366631306536626165356536227d9c7b2274797065223a2255496e7438222c2276616c7565223a2231227daf7b2274797065223a22537472696e67222c2276616c7565223a22666c6f772d6e6f64652e746573743a33353639227db89c7b2274797065223a22537472696e67222c2276616c7565223a223133343833303762633737633638386538303034396465396430383161613039373535646133336536393937363035666130353964623231343466633835653536306362653666376461386437346234353366353931363631386362386664333932633264623835366633653738323231646336386462316231643931346534227db8dc7b2274797065223a22537472696e67222c2276616c7565223a22396539616530643634356664356664393035303739326530623064616138326363313638366439313333616661306638316137383462333735633432616534383536376431353435653761396531393635663263316133326637336366383537356562623761393637663665346431303464326466373865623862653430393133356431326461303439396238613030373731663634326331623963343933393766323262343430343339663033366333626465653832663533303964616233227db07b2274797065223a22554669783634222c2276616c7565223a2239323233333732303336382e3534373735383038227db77b2274797065223a224f7074696f6e616c222c2276616c7565223a7b2274797065223a224172726179222c2276616c7565223a5b5d7d7da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a8899a8ac2c71d4f6bd040a8899a8ac2c71d4f6bdc98899a8ac2c71d4f6bdc0",
                "options": CryptoOptions(CurveChoice.Secp256k1, HashType.HASH_SHA2),
            },
            {
	        "tx": "f908b9f908b5b90534696d706f727420466c6f775374616b696e67436f6c6c656374696f6e2066726f6d203078393565303139613137643065323364370a0a2f2f2f2052656769737465727320612064656c656761746f7220696e20746865207374616b696e6720636f6c6c656374696f6e207265736f757263650a2f2f2f20666f722074686520737065636966696564206e6f646520696e666f726d6174696f6e20616e642074686520616d6f756e74206f6620746f6b656e7320746f20636f6d6d69740a0a7472616e73616374696f6e2869643a20537472696e672c0a202020202020202020202020726f6c653a2055496e74382c0a2020202020202020202020206e6574776f726b696e67416464726573733a20537472696e672c0a2020202020202020202020206e6574776f726b696e674b65793a20537472696e672c0a2020202020202020202020207374616b696e674b65793a20537472696e672c0a202020202020202020202020616d6f756e743a205546697836342c0a2020202020202020202020207075626c69634b6579733a205b537472696e675d3f29207b0a202020200a202020206c6574207374616b696e67436f6c6c656374696f6e5265663a2026466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e0a0a2020202070726570617265286163636f756e743a20417574684163636f756e7429207b0a202020202020202073656c662e7374616b696e67436f6c6c656374696f6e526566203d206163636f756e742e626f72726f773c26466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e3e2866726f6d3a20466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e53746f7261676550617468290a2020202020202020202020203f3f2070616e69632822436f756c64206e6f7420626f72726f772072656620746f205374616b696e67436f6c6c656374696f6e22290a0a20202020202020206966206c6574206d616368696e654163636f756e74203d2073656c662e7374616b696e67436f6c6c656374696f6e5265662e72656769737465724e6f6465280a20202020202020202020202069643a2069642c0a202020202020202020202020726f6c653a20726f6c652c0a2020202020202020202020206e6574776f726b696e67416464726573733a206e6574776f726b696e67416464726573732c0a2020202020202020202020206e6574776f726b696e674b65793a206e6574776f726b696e674b65792c0a2020202020202020202020207374616b696e674b65793a207374616b696e674b65792c0a202020202020202020202020616d6f756e743a20616d6f756e742c0a20202020202020202020202070617965723a206163636f756e7429200a20202020202020207b0a2020202020202020202020206966207075626c69634b657973203d3d206e696c207c7c207075626c69634b657973212e6c656e677468203d3d2030207b0a2020202020202020202020202020202070616e6963282243616e6e6f742070726f76696465207a65726f206b65797320666f7220746865206d616368696e65206163636f756e7422290a2020202020202020202020207d0a202020202020202020202020666f72206b657920696e207075626c69634b65797321207b0a202020202020202020202020202020206d616368696e654163636f756e742e6164645075626c69634b6579286b65792e6465636f64654865782829290a2020202020202020202020207d0a20202020202020207d0a202020207d0a7d0af9033bb85c7b2274797065223a22537472696e67222c2276616c7565223a2238383534393333356531646237623562343663326164353864646237306237613435653737306363356665373739363530626132366631306536626165356536227d9c7b2274797065223a2255496e7438222c2276616c7565223a2231227daf7b2274797065223a22537472696e67222c2276616c7565223a22666c6f772d6e6f64652e746573743a33353639227db89c7b2274797065223a22537472696e67222c2276616c7565223a223133343833303762633737633638386538303034396465396430383161613039373535646133336536393937363035666130353964623231343466633835653536306362653666376461386437346234353366353931363631386362386664333932633264623835366633653738323231646336386462316231643931346534227db8dc7b2274797065223a22537472696e67222c2276616c7565223a22396539616530643634356664356664393035303739326530623064616138326363313638366439313333616661306638316137383462333735633432616534383536376431353435653761396531393635663263316133326637336366383537356562623761393637663665346431303464326466373865623862653430393133356431326461303439396238613030373731663634326331623963343933393766323262343430343339663033366333626465653832663533303964616233227db07b2274797065223a22554669783634222c2276616c7565223a2239323233333732303336382e3534373735383038227db8e17b2274797065223a224f7074696f6e616c222c2276616c7565223a7b2274797065223a224172726179222c2276616c7565223a5b7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303665346634336637396433633164386361636233643566336537616565646232396665616562343535396664623731613937653266643034333835363533313065383736373030333564383362633130666536376665333134646261353336336338313635343539356436343838346231656361643135313261363465363565303230313634227d5d7d7da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a8899a8ac2c71d4f6bd040a8899a8ac2c71d4f6bdc98899a8ac2c71d4f6bdc0",
                "options": CryptoOptions(CurveChoice.Secp256k1, HashType.HASH_SHA2),
            },
        ]

        part = 0
        # Navigate in the main menu to change to expert mode
        util_set_expert_mode(firmware, navigator, f"{test_name}/part{part}")

        # Send the APDU and check the results
        for cfg in test_cfg:
            part += 1
            _check_transaction(client, firmware, navigator, f"{test_name}/part{part}", cfg["tx"], path, cfg["options"], "arbitrary")

