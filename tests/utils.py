from typing import List
import re
from pathlib import Path
from hashlib import sha256, sha3_256

from ecdsa.curves import Curve
from ecdsa.curves import SECP256k1, NIST256p
from ecdsa.keys import VerifyingKey
from ecdsa.util import sigdecode_der

from application_client.flow_command_sender import FlowCommandSender, Errors, HashType, CryptoOptions
from application_client.flow_response_unpacker import unpack_get_public_key_response

from ragger.bip import calculate_public_key_and_chaincode, CurveChoice
from ragger.conftest.configuration import OPTIONAL
from ragger.navigator import NavInsID, NavIns, Navigator
from ragger.firmware import Firmware

ROOT_SCREENSHOT_PATH = Path(__file__).parent.resolve()
TX_DOMAIN_TAG = "FLOW-V0.0-transaction"
MESSAGE_DOMAIN_TAG = "FLOW-V0.0-user"
DOMAIN_TAG_LENGTH = 32


def _init_header(signable_type: str) -> bytes:
    """ Prepare the message Header (DOMAIN_TAG) """

    if signable_type == "message":
        hdr = MESSAGE_DOMAIN_TAG.encode("utf-8").hex()
        pad_size = DOMAIN_TAG_LENGTH - len(MESSAGE_DOMAIN_TAG)
        hdr += bytearray([0] * pad_size).hex()
        return bytes.fromhex(hdr)

    hdr = TX_DOMAIN_TAG.encode("utf-8").hex()
    pad_size = DOMAIN_TAG_LENGTH - len(TX_DOMAIN_TAG)
    hdr += bytearray([0] * pad_size).hex()
    return bytes.fromhex(hdr)


def util_check_signature(
        public_key: bytes,
        signature: bytes,
        message: bytes,
        options: CryptoOptions,
        signable_type: str
) -> bool:
    """ Check if the signature of a given message is valid """

    # Convert curve value between bip to ecdsa
    ec_curve: Curve
    if options.curve == CurveChoice.Nist256p1:
        ec_curve = NIST256p
    elif options.curve == CurveChoice.Secp256k1:
        ec_curve = SECP256k1
    else:
        raise ValueError(f'Wrong Cruve "{options.curve}"')

    # Convert hash value to get the function
    if options.hash_t == HashType.HASH_SHA2:
        hashfunc = sha256
    elif options.hash_t == HashType.HASH_SHA3:
        hashfunc = sha3_256
    else:
        raise ValueError(f'Wrong Hash "{options.hash_t}"')

    key: VerifyingKey = VerifyingKey.from_string(public_key, ec_curve, hashfunc)

    # Prepare the message Header (DOMAIN_TAG)
    data = _init_header(signable_type) + message

    assert key.verify(signature, data, hashfunc, sigdecode_der)


def util_check_pub_key(
        client: FlowCommandSender,
        path: str,
        crypto_options: CryptoOptions,
) -> None:
    """ Retrieve and check the public key """

    # Send the APDU (Asynchronous)
    response = client.get_public_key_no_confirmation(path, crypto_options)
    assert response.status == Errors.SW_SUCCESS

    # Parse the response (Asynchronous)
    public_key = unpack_get_public_key_response(response.data)
    # Compute the reference data
    ref_public_key, _ = calculate_public_key_and_chaincode(crypto_options.curve, path, OPTIONAL.CUSTOM_SEED)
    # Check expected value
    assert public_key == ref_public_key

    return bytes.fromhex(public_key)


def util_set_slot(
        client: FlowCommandSender,
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path,
        slot: int,
        crypto_options: CryptoOptions = CryptoOptions(CurveChoice.Secp256k1, HashType.HASH_SHA2),
        address: str = "0000000000000000",
        path: str = "m/0/0/0/0/0",
) -> None:
    """ Function to Set Slot parameters """

    # Send the APDU (Asynchronous)
    with client.set_slot(slot=slot, address=address, path=path, crypto_options=crypto_options):
        util_navigate(firmware, navigator, test_name, "APPROVE_SLOT")

    # Check the status (Asynchronous)
    response = client.get_async_response()
    assert response.status == Errors.SW_SUCCESS


def util_set_expert_mode(
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path,
) -> None:
    """ Navigate in the menus to toggle Expert Mode """

    if firmware.device.startswith("nano"):
        instructions = [
            NavInsID.RIGHT_CLICK,
            NavInsID.BOTH_CLICK,
            NavInsID.LEFT_CLICK,
        ]
    else:
        instructions = [
            NavInsID.USE_CASE_HOME_SETTINGS,
            NavIns(NavInsID.TOUCH, (340, 128)),
            NavInsID.USE_CASE_SETTINGS_MULTI_PAGE_EXIT
        ]
    navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH, test_name, instructions,
                                   screen_change_before_first_instruction=False)


def util_navigate(
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path,
        text: str = "",
        timeout: int = 300,
) -> None:
    """ Navigate in the menus with conditions """

    assert text
    valid_instr = []

    if firmware.device.startswith("nano"):
        text = text.split("_")[0]
        nav_inst = NavInsID.RIGHT_CLICK
        valid_instr.append(NavInsID.BOTH_CLICK)

    else:
        if text.startswith("APPROVE"):
            if text == "APPROVE_PUBKEY":
                text = "Confirm"
                valid_instr.append(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CONFIRM)
            else:
                text = "Hold to sign"
                valid_instr.append(NavInsID.USE_CASE_REVIEW_CONFIRM)
            nav_inst = NavInsID.USE_CASE_REVIEW_TAP

        elif text.startswith("REJECT"):
            if text in ("REJECT_SIGN", "REJECT_SLOT"):
                text = r"Reject transaction\?"
                valid_instr.append(NavInsID.USE_CASE_CHOICE_CONFIRM)
            else:
                text = "Cancel"
                valid_instr.append(NavInsID.USE_CASE_CHOICE_REJECT)
            nav_inst = NavInsID.USE_CASE_REVIEW_REJECT

        else:
            raise ValueError(f'Wrong text "{text}"')

        valid_instr.append(NavInsID.USE_CASE_STATUS_DISMISS)

    navigator.navigate_until_text_and_compare(nav_inst,
                                              valid_instr,
                                              text,
                                              ROOT_SCREENSHOT_PATH,
                                              test_name,
                                              timeout)


def util_verify_name(name: str) -> None:
    """Verify the app name, based on defines in Makefile

    Args:
        name (str): Name to be checked
    """

    name_str = []
    lines = _read_makefile()
    name_re = re.compile(r"^APPNAME\s?=\s?\"?(?P<val>[ a-zA-Z0-9_]+)\"?", re.I)
    for line in lines:
        info = name_re.match(line)
        if info:
            dinfo = info.groupdict()
            name_str.append(dinfo["val"])
    assert name in name_str


def util_verify_version(version: str) -> None:
    """Verify the app version, based on defines in Makefile

    Args:
        Version (str): Version to be checked
    """

    vers_dict = {}
    vers_str = ""
    lines = _read_makefile()
    version_re = re.compile(r"^APPVERSION_(?P<part>\w)\s?=\s?(?P<val>\d*)", re.I)
    for line in lines:
        info = version_re.match(line)
        if info:
            dinfo = info.groupdict()
            vers_dict[dinfo["part"]] = dinfo["val"]
    try:
        vers_str = f"{vers_dict['M']}.{vers_dict['N']}.{vers_dict['P']}"
    except KeyError:
        pass
    assert version == vers_str


def _read_makefile() -> List[str]:
    """Read lines from the parent Makefile """

    parent = Path(__file__).parent.parent.resolve()
    makefile = f"{parent}/Makefile"
    with open(makefile, "r", encoding="utf-8") as f_p:
        lines = f_p.readlines()
    return lines
