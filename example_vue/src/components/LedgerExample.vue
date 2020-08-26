<template>
  <div class="Ledger">
    <!--
        Commands
    -->
    <button @click="getVersion">
      Get Version
    </button>

    <button @click="appInfo">
      AppInfo
    </button>

    <button @click="getAddress">
      Get Pubkey
    </button>

    <button @click="showAddress">
      Show Pubkey
    </button>

    <button @click="signExampleTx">
      Sign Example TX
    </button>
    <!--
        Commands
    -->
    <ul id="ledger-status">
      <li v-for="item in ledgerStatus" :key="item.index">
        {{ item.msg }}
      </li>
    </ul>
  </div>
</template>

<script>
import TransportWebUSB from "@ledgerhq/hw-transport-webusb";
import FlowApp from "@zondax/ledger-flow";

const scheme = 0x301;
const EXAMPLE_PATH = `m/44'/1'/${scheme}/0/0`;

export default {
  name: "Ledger",
  props: {},
  data() {
    return {
      deviceLog: [],
    };
  },
  computed: {
    ledgerStatus() {
      return this.deviceLog;
    },
  },
  methods: {
    log(msg) {
      this.deviceLog.push({
        index: this.deviceLog.length,
        msg,
      });
    },
    async getTransport() {
      let transport = null;

      this.log(`Trying to connect via WebUSB...`);
      try {
        transport = await TransportWebUSB.create();
      } catch (e) {
        this.log(e);
      }
      return transport;
    },
    async getVersion() {
      const transport = await this.getTransport();

      try {
        this.deviceLog = [];
        const app = new FlowApp(transport);

        // now it is possible to access all commands in the app
        this.log("Sending Request..");
        const response = await app.getVersion();
        if (response.returnCode !== FlowApp.ErrorCode.NoError) {
          this.log(`Error [${response.returnCode}] ${response.errorMessage}`);
          return;
        }

        this.log("Response received!");
        this.log(`App Version ${response.major}.${response.minor}.${response.patch}`);
        this.log(`Device Locked: ${response.deviceLocked}`);
        this.log(`Test mode: ${response.testMode}`);
        this.log("Full response:");
        this.log(response);
      } finally {
        transport.close();
      }
    },
    async appInfo() {
      const transport = await this.getTransport();
      try {
        this.deviceLog = [];
        const app = new FlowApp(transport);

        // now it is possible to access all commands in the app
        this.log("Sending Request..");
        const response = await app.appInfo();
        if (response.returnCode !== FlowApp.ErrorCode.NoError) {
          this.log(`Error [${response.returnCode}] ${response.errorMessage}`);
          return;
        }

        this.log("Response received!");
        this.log(response);
      } finally {
        transport.close();
      }
    },
    async getAddress() {
      const transport = await this.getTransport();
      try {
        this.deviceLog = [];
        const app = new FlowApp(transport);

        let response = await app.getVersion();
        this.log(`App Version ${response.major}.${response.minor}.${response.patch}`);
        this.log(`Device Locked: ${response.deviceLocked}`);
        this.log(`Test mode: ${response.testMode}`);

        // now it is possible to access all commands in the app
        this.log("Sending Request..");
        response = await app.getAddressAndPubKey(EXAMPLE_PATH);
        if (response.returnCode !== FlowApp.ErrorCode.NoError) {
          this.log(`Error [${response.returnCode}] ${response.errorMessage}`);
          return;
        }

        this.log("Response received!");
        this.log("Full response:");
        this.log(response);
      } finally {
        transport.close();
      }
    },
    async showAddress() {
      const transport = await this.getTransport();
      this.deviceLog = [];
      try {
        const app = new FlowApp(transport);

        let response = await app.getVersion();
        this.log(`App Version ${response.major}.${response.minor}.${response.patch}`);
        this.log(`Device Locked: ${response.deviceLocked}`);
        this.log(`Test mode: ${response.testMode}`);

        // now it is possible to access all commands in the app
        this.log("Sending Request..");
        this.log("Please click in the device");
        response = await app.showAddressAndPubKey(EXAMPLE_PATH);
        if (response.returnCode !== FlowApp.ErrorCode.NoError) {
          this.log(`Error [${response.returnCode}] ${response.errorMessage}`);
          return;
        }

        this.log("Response received!");
        this.log("Full response:");
        this.log(response);
      } finally {
        transport.close();
      }
    },
    async signExampleTx() {
      const transport = await this.getTransport();

      try {
        this.deviceLog = [];
        const app = new FlowApp(transport);

        let response = await app.getVersion();
        this.log(`App Version ${response.major}.${response.minor}.${response.patch}`);
        this.log(`Device Locked: ${response.deviceLocked}`);
        this.log(`Test mode: ${response.testMode}`);

        const message = Buffer.from("f90361f90339b89e7472616e73616374696f6e287075626c69634b6579733a205b5b55496e74385d5d29207b0a70726570617265287369676e65723a20417574684163636f756e7429207b0a6c65742061636374203d20417574684163636f756e742870617965723a207369676e6572290a666f72206b657920696e207075626c69634b657973207b0a616363742e6164645075626c69634b6579286b6579290a7d0a7d0a7df90256b902537b2274797065223a224172726179222c2276616c7565223a5b7b2274797065223a224172726179222c2276616c7565223a5b7b2274797065223a2255496e7438222c2276616c7565223a3230387d2c7b2274797065223a2255496e7438222c2276616c7565223a34307d2c7b2274797065223a2255496e7438222c2276616c7565223a3130337d5d7d2c7b2274797065223a224172726179222c2276616c7565223a5b7b2274797065223a2255496e7438222c2276616c7565223a3231307d2c7b2274797065223a2255496e7438222c2276616c7565223a3232317d2c7b2274797065223a2255496e7438222c2276616c7565223a3233357d2c7b2274797065223a2255496e7438222c2276616c7565223a3139327d5d7d2c7b2274797065223a224172726179222c2276616c7565223a5b7b2274797065223a2255496e7438222c2276616c7565223a3132337d2c7b2274797065223a2255496e7438222c2276616c7565223a3133357d2c7b2274797065223a2255496e7438222c2276616c7565223a38307d2c7b2274797065223a2255496e7438222c2276616c7565223a36377d2c7b2274797065223a2255496e7438222c2276616c7565223a3136387d2c7b2274797065223a2255496e7438222c2276616c7565223a3139317d2c7b2274797065223a2255496e7438222c2276616c7565223a3139377d2c7b2274797065223a2255496e7438222c2276616c7565223a33337d2c7b2274797065223a2255496e7438222c2276616c7565223a38347d2c7b2274797065223a2255496e7438222c2276616c7565223a3231307d5d7d5d7da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7e4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162", "hex");
        this.log("Sending Request..");
        response = await app.sign(EXAMPLE_PATH, message);

        this.log("Response received!");
        this.log("Full response:");
        this.log(response);
      } finally {
        transport.close();
      }
    },
  },
};
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
h3 {
  margin: 40px 0 0;
}

button {
  padding: 5px;
  font-weight: bold;
  font-size: medium;
}

ul {
  padding: 10px;
  text-align: left;
  alignment: left;
  list-style-type: none;
  background: black;
  font-weight: bold;
  color: greenyellow;
}
</style>
