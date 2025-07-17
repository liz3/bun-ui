import { toWindow, plot, generateFileSavePath  } from "../lib/index.mjs";

const c = plot(
    "Plot Title",
    [1 / 2],
    [
        [0, "0"],
        [1, "100"],
    ],
    [0, 50, 200],
    {
        width: 400,
        height: 400,
        spacing: 0
    },
);

await toWindow("Plot (save with S)", c);
