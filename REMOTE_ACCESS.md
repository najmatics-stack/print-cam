# Remote Access via Tailscale

Access your PrinterCam from anywhere using Tailscale subnet routing.

## How It Works

The ESP32-CAM can't run VPN software, so your Mac acts as a **subnet router** — it advertises your home network to Tailscale, letting your phone access the camera's local IP from anywhere.

```
Phone (anywhere)  -->  Tailscale network  -->  Mac (at home)  -->  ESP32-CAM (192.168.254.123)
```

---

## Setup

### 1. Install Tailscale on Your Mac

```bash
brew install tailscale
```

Or download from https://tailscale.com/download/mac

### 2. Sign In

Open the Tailscale app (menu bar icon) and sign in with Google, GitHub, or another provider.

### 3. Enable Subnet Routing on Your Mac

This tells Tailscale your Mac can route traffic to your home network:

```bash
sudo tailscale up --advertise-routes=192.168.254.0/24
```

> Replace `192.168.254.0/24` with your actual home subnet if different.

### 4. Approve the Subnet Route

1. Go to https://login.tailscale.com/admin/machines
2. Find your Mac in the list
3. Click the **...** menu → **Edit route settings**
4. Enable the `192.168.254.0/24` route

### 5. Install Tailscale on Your Phone

- **iOS**: Download "Tailscale" from the App Store
- **Android**: Download "Tailscale" from the Play Store

Sign in with the same account you used on your Mac.

### 6. Access the Camera Remotely

Open your phone's browser and go to:

```
http://192.168.254.123
```

It works exactly as if you were on your home WiFi.

---

## Requirements

- Your Mac must be **powered on and connected to your home WiFi** for remote access to work
- Both your Mac and phone must be signed into the **same Tailscale account**
- Tailscale free tier supports up to 100 devices — more than enough

---

## Troubleshooting

### Can't reach the camera remotely

1. Check that Tailscale is running on both your Mac and phone
2. Verify the subnet route is approved in the Tailscale admin panel
3. Make sure your Mac is on the same WiFi as the ESP32-CAM
4. Try `tailscale status` on your Mac to confirm it's connected

### Camera IP changed

If your router assigned a new IP to the ESP32-CAM, check the Serial Monitor for the new address. To prevent this, assign a **static IP** in your router's DHCP settings using the ESP32's MAC address (`1C:C3:AB:D1:C9:20`).

### Latency is high

Tailscale uses direct peer-to-peer connections when possible (via WireGuard). If latency is high, both devices may be relaying through Tailscale's DERP servers. Check with:

```bash
tailscale status
```

Direct connections show as "direct", relayed ones show the relay server name.
