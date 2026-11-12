# Sugarbaby Life 2 — RG Cube / Android

Sugarbaby Life 2 running on Android handhelds like the Anbernic RG Cube (720×720), and on phones too.

**Get the APK:** open the **Actions** tab → the newest green **Build APK** run → download **SugarbabyLife2-RGCube-APK** at the bottom. Unzip it and install `SugarbabyLife2-RGCube.apk`.

## Controls
| RG Cube | Game (3DS) |
|---|---|
| A (right) / B (bottom) / X (top) / Y (left) | A / B / X / Y |
| L1 / R1 | L / R |
| L2 / R2 | ZL / ZR |
| D-pad, left stick | D-pad, Circle Pad |
| Start / Select | START / SELECT |
| Touch the bottom picture | Touch screen |
| Back | B |

Saves are kept in the app's own storage, and the game saves by itself when you leave the app.

## How it's built
- `src/game` — the game itself (the same code as the 3DS version)
- `src/platform` — the 3DS system functions rebuilt on SDL2 (drawing, buttons, touch, sound, keyboard)
- `android` — the Android app wrapper; GitHub Actions builds the APK on every push
