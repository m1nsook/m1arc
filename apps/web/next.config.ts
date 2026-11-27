import type { NextConfig } from "next";

// Keep the verified local NextGuide configuration. Only Pages CI uses static export.
const pages = process.env.M1ARC_PAGES === "1";
const nextConfig: NextConfig = {
  ...(pages
    ? { output: "export", basePath: "/m1arc", trailingSlash: true, images: { unoptimized: true } }
    : { cacheComponents: true, partialPrefetching: true }),
  turbopack: {
    rules: {
      "*.css": {
        loaders: ["@tailwindcss/turbopack"],
        as: "*.css",
      },
    },
  },
};

export default nextConfig;
