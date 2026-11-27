import type { Metadata } from "next"
import type { ReactNode } from "react"
export const metadata: Metadata = { metadataBase: new URL("https://m1nsook.github.io/m1arc/"), title: { default: "M1 Arc", template: "%s | M1 Arc" }, description: "Native offline mod configuration and diagnostics for Cyberpunk 2077" }
export default function RootLayout({ children }: { children: ReactNode }) {
  return <html lang="en" suppressHydrationWarning><body>{children}</body></html>
}
