// Copyright GNU GPLv3 (c) 2023-2025 MoneroOcean <support@moneroocean.stream>

"use strict";

const path    = require("path");
const fast_rx = require(path.join(__dirname, 'index.js'));

// compute core wrapper for cluster process fork
if (fast_rx.cluster_process()) return;

let args = process.argv.slice(2);
const job          = JSON.parse(args.shift());
const result_hexes = args;

function exit(code) {
  fast_rx.messageWorkers({type: "close"});
  process.exitCode = code;
  return false;
}

// handles messages sent to the master thread from worker threads
function messageHandler(msg) {
  switch (msg.type) {
    case "test":
      const is_rx = job.algo.includes("rx/");
      // duplicate test result for batch size
      let result_hex2 = is_rx ? result_hexes[msg.value.rx_thread_id] : result_hexes[0];
      // for rx algos threads are encoded in batch
      const batch = fast_rx.get_dev_batch(fast_rx.get_thread_dev(msg.thread_id, job.dev));
      const rx_batch = is_rx ? 1 : batch;
      let result_hex3 = result_hex2;
      for (let i = 1; i < rx_batch; ++ i) result_hex3 += " " + result_hex2;
      if (msg.value.result !== result_hex3) {
        console.error("FAILED: " + msg.value.result + " != " + result_hex3);
	return exit(1);
      } else {
        console.log("PASSED");
        return exit(0);
      }

    case "error":
      console.error("Compute core error: " + JSON.stringify(msg.value));
      return exit(1); // exit with error

    default:
      console.error("Unknown master thread message: " + JSON.stringify(msg));
  }
}
fast_rx.create_thread(messageHandler);
fast_rx.messageWorkers({type: "test", job: job});
