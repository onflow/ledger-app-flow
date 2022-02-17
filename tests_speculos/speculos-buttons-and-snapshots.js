import { syncBackTicks, sleep, humanTime } from "./speculos-common.js"


const flowReadySHANanoS = "sha256:f3916e7cbbf8502b3eedbdf40cc6d6063b90f0e4a4814e34f2e7029bdaa4eaac";
const flowReadySHANanoX = "sha256:186628eff908796f25c78a0f6ec6ec7ceef5ae6c92e9d240e8196bf34dec7688";

const approveSHANanoS = "sha256:bf2dd4e939fc90cde614b84e97e7f674491063a78b2b47c6faa3639b45b2b48d";
const approveSHANanoX = "sha256:acdd16e95aca0642489009b3ee2a5da2823597a89f227d541ce1c6fdde064356";

const max_review_screens = 100;

class ButtonsAndSnapshots {
    scriptName;
    pngNum = 1;
    pngSha256Previous = "";
    speculosButtonsPort;
	isNanoX;

	flowReadySHA;
	approveSHA;
    
    constructor(scriptName, conf) {
        this.scriptName = scriptName;
        this.speculosButtonsPort = conf.speculosApiPort;
		this.isNanoX = conf.isNanoX;
		this.flowReadySHA = this.isNanoX ? flowReadySHANanoX : flowReadySHANanoS;
		this.approveSHA = this.isNanoX ? approveSHANanoX : approveSHANanoS;
    }

	curlButton(which, hint) { // e.g. which: 'left', 'right', or 'both'
		console.log(humanTime() + " curlButton() // " + which + hint);
		const output = syncBackTicks('curl --silent --show-error --max-time 60 --data \'{"action":"press-and-release"}\' http://127.0.0.1:' + this.speculosButtonsPort + '/button/' + which + ' 2>&1');
		if (output != '{}') {
			console.log(humanTime() + " ERROR: unexpected curl stdout: " + output);
			throw new Error();
		}
	}

	async curlButtonAndScreenshot(which, hint) {
		this.curlButton(which, hint);
		return await this.curlScreenShot(which);
	}

	async curlScreenShot(lastButton = "") {
		const test_device = this.isNanoX ? "nanox":"nanos"; 
		// e.g. test-transactions.staking-sign-ts.02-transfer-top-shot-moment-p256-sha3-256/nanos.01.png
		const png = this.scriptName.replace(".js", "") + "/" + test_device + "." + this.pngNum.toString(10).padStart(2, '0') + ".png"
		console.log(humanTime() + " curlScreenShot() // " + png + ".new.png");

		const makeScreenshot = (process.env.TEST_PNG_RE_GEN_FOR && (this.scriptName.substring(0, process.env.TEST_PNG_RE_GEN_FOR.length) == process.env.TEST_PNG_RE_GEN_FOR));
		const oldSHAcmd = makeScreenshot ? "" : "echo sha256:`sha256sum $PNG`";

        let generateNewScreenshotFromNextCapture = 0;
		let loops = 0;
		do {
			// get screenshot
			const output = syncBackTicks('export PNG=' + png + ' ; curl --silent --show-error --output $PNG.new.png http://127.0.0.1:' + this.speculosButtonsPort + '/screenshot 2>&1 ; echo sha256:`sha256sum $PNG.new.png` ; '+ oldSHAcmd);

			const errorArray = output.match(/Empty reply from server/gi);
			if (null != errorArray) {
				console.log(humanTime() + " curl: screen shot: warning: curl failed to grab screen shot");
				throw new Error();
			}
			const regex = /sha256:[^\s]*/gm;
			const sha256Array = output.match(regex); // e.g. ['sha256:f3916e7cbbf8502b3eedbdf40cc6d6063b90f0e4a4814e34f2e7029bdaa4eaac','sha256:f3916e7cbbf8502b3eedbdf40cc6d6063b90f0e4a4814e34f2e7029bdaa4eaac']

			// verify, that the screenshot is not the same as previous one
			if (sha256Array[0] /* newly generated PNG */ == this.pngSha256Previous) {
				loops += 1;
				generateNewScreenshotFromNextCapture = 0
				if (loops < 20) {
					if (loops == 15 && lastButton!="") {
						await sleep(100);
						console.log(humanTime() + " Retrying last button press: " +lastButton);
						this.curlButton(lastButton, " Retry last button press.");
						await sleep(100);
					}
					console.log(humanTime() + " curlScreenShot() // matches previous screen shot SHA256 (" + this.pngSha256Previous + "); so requesting another screen shot");
					await sleep(90+10*loops)
					continue;
				} else {
					console.log(humanTime() + " curlScreenShot() // matches previous screen shot SHA256 (" +  + "); ERROR: giving up because too many tries; curl one-liner output:");
					console.log(output);
					console.log(png);
					console.log(humanTime() + " curlScreenShot() // NOTE: re-run with TEST_PNG_RE_GEN_FOR=" + this.scriptName + " to regenerate PNGs");
					throw new Error();
				}
			}

			// if we generate this screenshot ...
			if (makeScreenshot) {
				// the screenshot we have may be partial capture
				// the tests made suggest, that when we make another screenshot, it will be OK
				if (generateNewScreenshotFromNextCapture == 0) {
					generateNewScreenshotFromNextCapture = 1;
					continue;
				}
				this.pngSha256Previous = sha256Array[0];

				// second try, we believe the screenshot is correct
				generateNewScreenshotFromNextCapture = 0;
				syncBackTicks('export PNG=' + png + ' ; cp $PNG.new.png $PNG');
				break;
			}
			// if we want to compare this screenshot
			else {
				this.pngSha256Previous = sha256Array[0];
			
				// if we have it, we are done
				if (sha256Array[0] == sha256Array[1]) {
					break;
				}
				// if we want to ignore the test we are done
				if (process.env.TEST_IGNORE_SHA256_SUMS >= 1) {
					console.log(humanTime() + " curlScreenShot() // running tests with TEST_IGNORE_SHA256_SUMS=1 to ignore all PNG differences");
					break;
				}
				// otherwise, we will try again (this deals with partial capture)
				loops += 1;
				if (loops < 20) {
					await sleep(100+10*loops)
					console.log(humanTime() + " curlScreenShot() // screen shot: warning: sha256 sums are different; could be partially rendered screen, so re-requesting screen shot // re-run with TEST_IGNORE_SHA256_SUMS=1 to ignore all PNGs");
					this.pngSha256Previous = sha256Array[0];
					continue;
				} else {
					console.log(humanTime() + " curlScreenShot() // screen shot: warning: sha256 sums are different; ERROR: re-requested screen shot too many times // re-run with TEST_IGNORE_SHA256_SUMS=1 to ignore all PNGs");
					console.log(png);
					throw new Error();
				}
			}
		} while (true);
		console.log(humanTime() + " png " + this.pngNum + " captured, SHA: " + this.pngSha256Previous + ".");
		this.pngNum++;
		return this.pngSha256Previous;
	}

    //higher level functions to be used in the tests
    //Each step should end on "Flow Ready" screen
    checkFlowReadyScreen(sha) {
		if (sha != this.flowReadySHA) {
			throw new Error();
		}
	}

    async makeStartingScreenshot() {
        console.log(humanTime() + " screen shot before sending first apdu command");
        const sha = await this.curlScreenShot();
		this.checkFlowReadyScreen(sha);
    }

    async review(textWhat) {
        await this.curlScreenShot();
        for(let i=1; i<max_review_screens; i++) {
            const sha = await this.curlButtonAndScreenshot("right", " Scroll "+textWhat+" [from screen "+i+" to "+(i+1)+"]");
			if (sha == this.approveSHA) {
				console.log(humanTime() + " approve detected.");
				break;
			}
        }
        await this.curlButtonAndScreenshot("both", "Approve: "+textWhat);
        console.log(humanTime() + " back to main screen");
    }

    async toggleExpertMode(textWhat) {       
        await this.curlButtonAndScreenshot("right", "Menu: Flow Ready -> Expert Mode");
        await this.curlButtonAndScreenshot("both", "Toggle expert mode: " + textWhat);        
        await this.curlButtonAndScreenshot("left", "Menu: Flow Ready -> Expert Mode");
        console.log(humanTime() + " back to main screen");
    }

    async enterMenuElementAndReview(menuElementIndex, textWhat) {
        for(const i of Array.from({length: menuElementIndex-1}, (_, i) => i + 1)) {
            await this.curlButtonAndScreenshot("right", " Scroll menu [from screen "+i+" to "+(i+1)+" out of "+(menuElementIndex)+"]");
        }
        await this.curlButtonAndScreenshot("both", "Enter Menu: "+textWhat);
        //When you enter menu element, you already made screenshot of first review screen
        //This is necessary to confirm that the button click was catched by Speculos
		//Thus this workflow is slightly different then the review one...
        for(let i=1; i<max_review_screens; i++) {
            const sha = await this.curlButtonAndScreenshot("right", " Scroll "+textWhat+" [from screen "+i+" to "+(i+1)+"]");
			if (sha == this.approveSHA) {
				console.log(humanTime() + " approve detected.");
				break;
			}
        }
        await this.curlButtonAndScreenshot("both", "Approve: "+textWhat);
        console.log(humanTime() + " back to main screen");
    }

}

export {ButtonsAndSnapshots};
