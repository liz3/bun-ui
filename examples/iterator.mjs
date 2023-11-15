import { iterativeWindow, plot } from "../lib/index.mjs";

await iterativeWindow(
    "Window title",
    400,
    400,
    (givenIndex) => {
        const { canvas, w, h } = plot(
            "Plot Title",
            [1 / givenIndex],
            [
                [0, "0"],
                [1, "100"],
            ],
            [0, 50, 200],
            {
                width: 400,
                height: 400,
            },
        );
        return { buffer: canvas.toBuffer("raw"), w, h };
    },
    1,
);
