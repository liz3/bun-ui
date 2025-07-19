import {pie, toWindow} from '../lib/index.mjs';

await toWindow("Pie test", pie("Test", [[0.3, "First"], [0.3, "Second"], [0.4, "Third"]]))