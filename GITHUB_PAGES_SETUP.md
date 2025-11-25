# GitHub Pages Setup Instructions

## Enable GitHub Pages for Web Flasher

Follow these steps to make the web flasher publicly accessible:

### 1. Push the Branch to GitHub

```bash
git push origin feature/code-quality-and-enhancements
```

### 2. Enable GitHub Pages

1. Go to your repository on GitHub: `https://github.com/PokeyPoke/ESP32-DataTracker`
2. Click **Settings** (top right)
3. Scroll down to **Pages** (in the left sidebar under "Code and automation")
4. Under **Source**, select:
   - **Branch:** `feature/code-quality-and-enhancements`
   - **Folder:** `/docs`
5. Click **Save**

### 3. Wait for Deployment

GitHub will automatically build and deploy your site. This takes 1-3 minutes.

### 4. Access Your Web Flasher

Once deployed, your web flasher will be available at:

**https://pokeypoke.github.io/ESP32-DataTracker/**

Or more specifically:

**https://pokeypoke.github.io/ESP32-DataTracker/flash-v2.html**

### 5. Verify the Firmware Files

The web flasher will load firmware from:
- `https://pokeypoke.github.io/ESP32-DataTracker/firmware/bootloader.bin`
- `https://pokeypoke.github.io/ESP32-DataTracker/firmware/partitions.bin`
- `https://pokeypoke.github.io/ESP32-DataTracker/firmware/boot_app0.bin`
- `https://pokeypoke.github.io/ESP32-DataTracker/firmware/firmware.bin`

All files are present and correctly referenced in `manifest.json`.

## Troubleshooting

### "Firmware files not found" error

If the web flasher can't find firmware files:

1. Check that GitHub Pages is enabled for the `/docs` folder
2. Verify the deployment completed (look for green checkmark in Actions tab)
3. Ensure all 4 `.bin` files exist in `docs/firmware/` directory
4. Check that `docs/manifest.json` paths are relative (no leading `/`)

### The page shows old content

1. GitHub Pages may cache content. Wait 5-10 minutes
2. Try hard refresh: Ctrl+Shift+R (Windows/Linux) or Cmd+Shift+R (Mac)
3. Clear your browser cache

### Deploy from a Different Branch

If you want to deploy from `main` instead:

1. Merge the feature branch into main:
   ```bash
   git checkout main
   git merge feature/code-quality-and-enhancements
   git push origin main
   ```

2. Change GitHub Pages source to `main` branch, `/docs` folder

## Current Configuration

- **Repository:** https://github.com/PokeyPoke/ESP32-DataTracker
- **Branch:** feature/code-quality-and-enhancements
- **Source Folder:** /docs
- **Firmware Version:** 3.3.0-enhanced-v1.0
- **Manifest:** docs/manifest.json
- **Web Flasher:** docs/flash-v2.html
- **Landing Page:** docs/index.html (redirects to flash-v2.html)

## Files in /docs Directory

```
docs/
├── index.html              # Landing page (redirects to flasher)
├── flash-v2.html           # Main web flasher
├── manifest.json           # Firmware manifest for ESP Web Tools
├── settings_v2.html        # Settings interface (optional)
├── firmware/
│   ├── bootloader.bin      # 13 KB
│   ├── partitions.bin      # 3 KB
│   ├── boot_app0.bin       # 8 KB
│   ├── firmware.bin        # 1.4 MB
│   └── enhanced-v1.0/      # Backup copy
└── README.md
```

## Next Steps

After GitHub Pages is live:

1. **Test the web flasher** with an actual ESP32-C3 device
2. **Share the link** with users
3. **Update documentation** with the GitHub Pages URL
4. **Consider custom domain** (optional)

## Custom Domain (Optional)

To use a custom domain like `flash.datatracker.dev`:

1. Add a `CNAME` file to `/docs` with your domain
2. Configure DNS records at your domain registrar
3. Update GitHub Pages settings with custom domain

See: https://docs.github.com/en/pages/configuring-a-custom-domain-for-your-github-pages-site
