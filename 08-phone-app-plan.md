# Phone transfer: getting photos onto an iPhone

## Built 15 Oct: Phone mode, no app and no fees

You chose not to pay for the Apple Developer Program, so this replaces the app plan further down.

**Why not Bluetooth straight to Photos:** an iPhone doesn't accept files over Bluetooth from other devices. There's no Bluetooth file receiving, and AirDrop only works between Apple devices. The only way in over Bluetooth is your own app, and putting an app on someone else's iPhone needs the paid program.

**What was built instead:** the camera makes its own Wi-Fi network and serves a photo page. Safari saves photos to the Photos app.

1. **On the camera:** hold ●, choose **Phone**. The screen shows a QR code with the Wi-Fi name and password. Shutter or hold ● turns it off. ● switches between the two QR codes by hand.
2. **On the iPhone:**
   - Point the Camera app at the QR code and tap **Join**.
   - The camera's screen changes to a second QR code: scan it, and Safari opens the photos at http://gallery.local (since 0.4.3; http://192.168.4.1 works too).
3. **Save one photo:** tap it, press and hold, tap **Save to Photos**. No setup at all.
4. **Save many at once:** tap **Select** (or **Save N new**). A free Shortcut called "Save from Digicam", set up once, downloads them all into Photos.
   - The page shows the setup steps the first time.
   - After you've made the Shortcut on your phone, share it to your friend as an iCloud link (Shortcuts → share → Copy iCloud Link). Then she adds it with one tap.
5. **Ticks:** the page remembers what's been saved on that phone and ticks it. "Save N new" only takes new ones.
6. **Delete:** open a photo, tap **Delete**, then confirm.
7. **Videos:** they play on the page. Saving videos to Photos isn't done yet, because iPhones can't play the camera's AVI files; that needs converting first.

| Part | Where |
| --- | --- |
| Photo page (also served by the laptop simulator at `/phone/`) | `firmware/web/phone.html` |
| Wi-Fi network and web server | `firmware/main/phone.c` |
| Phone screen and QR codes | `ui_draw.c` (`draw_phone`), `main.c` (`phone_*`) |

- **Wi-Fi name:** `digicam-<owner>` from `personal.h`, or `digicam-xxxx` if no name is set.
- **Password:** made once on first use and kept in the camera's flash. The network uses WPA2, so only someone who can see the camera's screen can join.
- **While a phone is connected:** it has no internet. iOS shows "No Internet Connection" under the network; that's expected.

**Still to do:**

- Test on a real iPhone.
- Saving videos: record .MOV instead of AVI if iPhones accept it, or convert on the phone.
- The page could show the camera's battery.

---

# Earlier plan (15 Oct, before the no-fee decision)

15 Oct 2026. **A plan only. Nothing is built until you've agreed the decisions below.**

**The job:** your friend turns the camera to a "Phone" mode and opens an app on her iPhone. The app shows what's on the card, and the photos and videos land in her Photos library, in a "Digicam" album.

## What the iPhone allows

These shape everything else:

1. **Bluetooth on an iPhone means Bluetooth Low Energy (BLE).** iPhone apps can't use classic Bluetooth serial links unless the device is Apple-certified (MFi). That's fine here: the ESP32-S3 only has BLE anyway.
2. **BLE is slow for photos.** An ESP32-S3 talking to an iPhone typically moves **about 50–150 KB/s** (estimate; stage 1 measures it).

   | Item | Typical size | Time over BLE (estimate) |
   | --- | --- | --- |
   | Thumbnail | about 10 KB | under 0.2 s |
   | Full photo, 2048 × 1536 | 250–400 KB | about 2–8 s |
   | 30 photos | | about 1–4 min |
   | 1-minute video | about 17 MB | about 2–6 min |

   Wi-Fi from the camera would be roughly 5–10× faster. Camera makers (Fujifilm, Canon, Ricoh) use BLE to find and pair the camera, then switch to Wi-Fi for the files.
3. **Installing an app on someone else's iPhone needs an Apple Developer account ($99 USD a year, about S$130).** This is the biggest decision.

   | Route | What it means for a gift | Cost |
   | --- | --- | --- |
   | Your free Apple ID, through Xcode | The app **stops working after 7 days** and must be reinstalled from your Mac. Fine for building and testing on your own phone; not for a gift | Free |
   | Apple Developer Program, TestFlight | Your friend installs Apple's TestFlight app, then yours from an invite link. Each build lasts 90 days; you upload a new one before it expires | $99 USD a year |
   | Apple Developer Program, Ad Hoc | You register her phone's ID; the app lasts a year | $99 USD a year |
   | App Store | Public listing, Apple review. Overkill for two friends | $99 USD a year |

   The $99 alone is more than the per-camera budget target, though it covers both cameras and every future build.
4. **No app at all is possible.** The camera can make its own Wi-Fi network and serve a web page. Your friend joins "digicam" Wi-Fi, opens Safari, and saves photos from the page. It's free, works on any phone, and is fast. It's clunkier, though: join Wi-Fi, open a page, save photos one by one or through the share sheet, and the phone has no internet while connected. **iPhone Safari doesn't support Web Bluetooth**, so a no-app route has to be Wi-Fi.
5. **iPhones can't play the camera's videos.** The camera records MJPEG in an AVI file. A native app can convert each video to an MP4 on the phone before saving it to Photos. With the web-page route, videos need a converter, or a firmware change worth testing: recording into a .MOV file instead of AVI.

## Recommendation

**A native iPhone app (Swift and SwiftUI), BLE first, plus the Apple Developer Program for TestFlight.**

- **Swift and SwiftUI:** native Bluetooth (CoreBluetooth) is the most reliable on iPhone, and the video conversion (AVFoundation) and saving to Photos are built in.
- **Not Expo:** your lovebox notes already flag Bluetooth library compatibility problems with newer Expo versions, and it would still need a custom build and the same paid account.
- **BLE first:** it's simpler and enough for photos. Wi-Fi fast transfer is an optional later stage, if videos turn out too slow.
- **The web-page route** stays the fallback if you'd rather not pay.

## Stages

Each stage ends with something you can try. I'll stop after each one for you to check.

| Stage | What gets built | You can try | Size |
| --- | --- | --- | --- |
| **1. Camera Bluetooth service** | Firmware: a "Phone" tile on the mode dial turns Bluetooth on. The screen shows "Connect from your phone" and a 6-digit code. Phones must pair with that code first, so nobody nearby can pull photos. The service lists files, sends thumbnails and files (resumable), and reports battery and card. A laptop test script on your Mac measures the real speed | Phone mode on the camera; speed numbers for photos and video | 1–2 days |
| **2. App: find and browse** | Xcode project; scan for the camera; pair (iOS shows its own code-entry box); a grid of thumbnails, newest first, with camera battery and card space. Installed on **your** iPhone from Xcode with your free Apple ID | Browse the camera's photos on your phone | 2 days |
| **3. App: import** | "Import new" and single-photo import into a "Digicam" album. Shows progress, resumes after a dropped connection, remembers what's already imported. Optional: delete from the camera after import | Photos landing in your Photos app | 1–2 days |
| **4. Videos** | Download the AVI, convert to MP4 on the phone, save it. Play button in the grid | Videos in Photos, playable | 1 day |
| **5. Nice touches** | The phone sets the camera's clock on connect, enabling real dates on files and an optional retro date stamp on photos. Owner name and settings (film look, self-timer) from the phone. App icon and name to match the camera | A finished-feeling app | 1–2 days |
| **6. Hand over** | TestFlight setup (paid account), an install note for your friend, and a short "how to use" card | Your friend installs it | half a day |
| *Optional W. Wi-Fi fast import* | The camera starts its own Wi-Fi for big imports; the app joins it automatically and downloads 5–10× faster | Videos in seconds rather than minutes | 2 days |

Stage 1 doesn't depend on any app decision, and I can test it fully from your Mac. Stages 2 to 4 can be built and used on your own phone with a free Apple ID; the paid account is only needed at stage 6.

## How the app will look (stage 2–4)

- **Camera screen:**
  - Header: camera name, battery %, card space, a "Connected" pill.
  - A 3-column grid of thumbnails, newest first; videos show their length; imported ones carry a small tick.
  - Bottom bar: "Import 12 new" as the main button.
- **Photo view:** full photo, swipe between them, with "Save" and "Delete from camera" buttons.
- **Not connected:** a picture of the camera with "Turn the camera to Phone mode (hold ●, choose Phone)".

## Camera side: what changes in the firmware

- **Mode dial:** a fourth tile, **Phone**. Bluetooth runs only in this mode: it saves battery and keeps the camera private the rest of the time.
- **Phone screen:** "Connect from your phone", the camera's name (for example "digicam-maya") and the pairing code. After pairing: "Connected to Maya's iPhone", a transfer progress bar, and "Press shutter to stop".
- **The BLE service** (detail in stage 1):
  - status;
  - list of files (name, size, video or photo);
  - fetch thumbnail;
  - fetch file from an offset, so a dropped transfer resumes;
  - delete;
  - set clock and owner name.

  Same commands as the laptop link, so the laptop simulator and the app share one design.

## Decisions needed from you

1. **App install route:** pay for the Apple Developer Program (TestFlight), or go no-app (camera Wi-Fi plus a Safari page)?
2. **Transfer:** Bluetooth only, or Bluetooth plus the optional Wi-Fi fast-import stage?
3. **Framework:** native Swift (recommended), or Expo / React Native because you know it?
4. **Delete after import:** should importing remove photos from the camera's card automatically, or only when she taps delete?
