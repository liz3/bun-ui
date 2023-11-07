import Window from "../lib/index.mjs";

for (let i = 0; i < 5; i++) {
    const window = new Window("test " + i, 200, 200);
    window.setCloseCallback(() => {
        console.log("done");
    });
    window.create();
    window.setClearColor(200, 200, 200);
    window.setTextCallback((code) => {
        console.log("cp", i, code);
    });
    window.setKeyCallback((data) => {
        console.log("key", i, data);
    });
}
