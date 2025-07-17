import { spawn } from "node:child_process";
import fs from "node:fs";

if (process.argv.includes("delbuild")) {
  fs.rmdirSync("build", { recursive: true });
} else if (process.argv.includes("mkbuild")) {
  if (fs.existsSync("build")) fs.rmdirSync("build", { recursive: true });

  fs.mkdirSync("build");
} else {
  let proc;
  if (process.platform === "win32")
    proc = spawn("cmake", ["--build", ".", "--config", "Release"]);
  else proc = spawn("make");

  proc.stdout.pipe(process.stdout);
  proc.stderr.pipe(process.stderr);

  proc.on("exit", (c) => process.exit(c));
}
