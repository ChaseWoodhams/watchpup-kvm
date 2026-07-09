import { defineConfig } from "vite";

export default defineConfig({
  build: {
    outDir: "../firmware/generated/web",
    emptyOutDir: true,
  },
});
