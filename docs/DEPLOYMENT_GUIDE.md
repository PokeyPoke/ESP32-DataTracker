# Deployment Guide - Web Flasher

Complete guide to deploying your web flasher to GitHub Pages.

## 📋 Prerequisites

- GitHub account
- Git installed locally
- Firmware built successfully
- `docs/` folder with web flasher files

---

## 🚀 Quick Deployment Steps

### 1. Build the Firmware Binaries

First, generate the flashable binaries:

**Linux/macOS:**
```bash
./scripts/build_web_flasher.sh
```

**Windows:**
```batch
scripts\build_web_flasher.bat
```

This creates all necessary files in `docs/firmware/`:
- `bootloader.bin`
- `partitions.bin`
- `boot_app0.bin`
- `firmware.bin`

### 2. Commit the Files

```bash
# Add all docs files
git add docs/

# Commit with descriptive message
git commit -m "Add web flasher with v1.0.0 firmware"

# Push to GitHub
git push origin main
```

### 3. Enable GitHub Pages

1. Go to your repository on GitHub
2. Click **Settings** (top menu)
3. Scroll down to **Pages** (left sidebar)
4. Under **Source**:
   - Branch: Select **main** (or **master**)
   - Folder: Select **/docs**
   - Click **Save**

5. Wait 1-2 minutes for deployment

### 4. Access Your Web Flasher

Your flasher will be available at:
```
https://YOUR-USERNAME.github.io/YOUR-REPO-NAME/flash.html
```

Example:
```
https://john-doe.github.io/DataTracker/flash.html
```

---

## 🎯 Updating URLs in Files

After deploying, update the URL references in your files:

### 1. Update `README.md`

Replace:
```markdown
[Flash Firmware](https://yourusername.github.io/DataTracker/flash.html)
```

With your actual URL:
```markdown
[Flash Firmware](https://YOUR-USERNAME.github.io/YOUR-REPO-NAME/flash.html)
```

### 2. Update `docs/flash.html`

Replace:
```html
<a href="https://github.com/yourusername/DataTracker" ...>
```

With your actual repository URL.

### 3. Commit and Push Changes

```bash
git add README.md docs/flash.html
git commit -m "Update web flasher URLs"
git push
```

---

## 🧪 Testing Locally Before Deployment

Always test locally before pushing to GitHub Pages:

### Method 1: Python HTTP Server

```bash
cd docs
python3 -m http.server 8000
```

Open: `http://localhost:8000/flash.html`

### Method 2: VS Code Live Server

1. Install "Live Server" extension
2. Right-click `docs/flash.html`
3. Select "Open with Live Server"

### What to Test

- ✅ Page loads correctly
- ✅ Browser compatibility message shows if needed
- ✅ Install buttons are visible
- ✅ manifest.json loads (check browser console)
- ✅ Firmware binaries exist in `firmware/` folder
- ✅ Can connect to ESP32-C3 and see port selection
- ✅ Flash completes successfully

---

## 🔄 Updating Firmware on Deployed Site

When you release new firmware:

### 1. Build New Binaries

```bash
# Update version in platformio.ini or code
nano platformio.ini  # Change version

# Build
pio run

# Generate web flasher binaries
./scripts/build_web_flasher.sh
```

### 2. Update Version in Manifest

Edit `docs/manifest.json`:
```json
{
  "name": "ESP32-C3 Data Tracker",
  "version": "1.1.0",  // ← Update this
  ...
}
```

### 3. Commit and Push

```bash
git add docs/firmware/ docs/manifest.json
git commit -m "Update firmware to v1.1.0"
git push
```

GitHub Pages will automatically update within 1-2 minutes!

---

## 🏷️ Using GitHub Actions (Automated)

The included GitHub Actions workflow automatically builds binaries on release tags.

### Create a Release

```bash
# Tag your release
git tag -a v1.0.0 -m "Release v1.0.0"
git push origin v1.0.0
```

GitHub Actions will:
1. Build the firmware
2. Generate web flasher binaries
3. Create a GitHub Release
4. Deploy to GitHub Pages

### View Build Status

- Go to **Actions** tab in your repository
- Watch the build progress
- Download artifacts if needed

---

## 🌐 Custom Domain (Optional)

Want a custom URL like `flash.myproject.com`?

### 1. Add CNAME File

Create `docs/CNAME` with your domain:
```
flash.myproject.com
```

### 2. Configure DNS

Add a CNAME record in your DNS provider:
```
CNAME  flash  YOUR-USERNAME.github.io
```

### 3. Enable HTTPS

GitHub Pages automatically provides HTTPS for custom domains.

---

## 🔍 Troubleshooting Deployment

### Issue: 404 Not Found

**Cause:** GitHub Pages not enabled or wrong folder selected

**Solution:**
1. Check Settings → Pages
2. Verify source is `/docs` folder
3. Wait 2-3 minutes for deployment
4. Clear browser cache

### Issue: Binaries Not Loading

**Cause:** Firmware files missing or wrong path

**Solution:**
1. Check `docs/firmware/` has all 4 .bin files
2. Verify `manifest.json` paths are correct
3. Check browser console for 404 errors
4. Ensure files are committed and pushed

### Issue: Serial Port Not Detected

**Not a deployment issue** - this is browser/driver related. See [WEB_FLASHER_GUIDE.md](WEB_FLASHER_GUIDE.md) troubleshooting.

### Issue: Changes Not Showing Up

**Cause:** Browser cache or GitHub Pages cache

**Solution:**
1. Wait 2-3 minutes after push
2. Hard refresh browser (Ctrl+Shift+R / Cmd+Shift+R)
3. Try incognito/private mode
4. Check GitHub Actions deployment status

---

## 📊 Monitoring Usage

GitHub doesn't provide analytics by default, but you can add:

### Google Analytics

Add to `docs/flash.html` before `</head>`:
```html
<!-- Google Analytics -->
<script async src="https://www.googletagmanager.com/gtag/js?id=G-XXXXXXXXXX"></script>
<script>
  window.dataLayer = window.dataLayer || [];
  function gtag(){dataLayer.push(arguments);}
  gtag('js', new Date());
  gtag('config', 'G-XXXXXXXXXX');
</script>
```

### Plausible Analytics (Privacy-Friendly)

```html
<script defer data-domain="yourdomain.com" src="https://plausible.io/js/script.js"></script>
```

---

## 🔒 Security Best Practices

### 1. Protect Your Repository

- Don't commit API keys or secrets
- Use `.gitignore` for sensitive files
- Review all files before pushing

### 2. Firmware Signing (Advanced)

For production:
- Sign firmware binaries
- Implement version checking
- Add rollback capability

### 3. HTTPS Only

- GitHub Pages provides free HTTPS
- Enforce HTTPS in repository settings
- Update all links to use `https://`

---

## 📈 Advanced: Multiple Versions

Host multiple firmware versions:

```
docs/
├── flash.html              # Latest version
├── v1.0.0/
│   ├── flash.html
│   └── firmware/
├── v1.1.0/
│   ├── flash.html
│   └── firmware/
└── latest/ → v1.1.0       # Symlink
```

Update `manifest.json` in each version folder.

---

## 🎓 Learning Resources

- **GitHub Pages Docs**: https://docs.github.com/pages
- **ESP Web Tools**: https://esphome.github.io/esp-web-tools/
- **Web Serial API**: https://developer.chrome.com/articles/serial/

---

## ✅ Deployment Checklist

Before announcing your web flasher:

- [ ] Firmware builds without errors
- [ ] All 4 binary files generated
- [ ] Tested locally (http://localhost:8000/flash.html)
- [ ] manifest.json has correct version and paths
- [ ] GitHub Pages enabled with /docs folder
- [ ] Deployment successful (check Actions tab)
- [ ] Web flasher loads at GitHub Pages URL
- [ ] Can connect to ESP32-C3 device
- [ ] Flash process completes successfully
- [ ] Device creates WiFi AP after flash
- [ ] Configuration portal works
- [ ] URLs updated in README.md and flash.html
- [ ] Documentation mentions web flasher
- [ ] Added link in project README

---

## 💬 Need Help?

- **GitHub Pages Issues**: https://github.com/orgs/community/discussions/categories/pages
- **Project Issues**: [Your repo's issues page]
- **ESP Web Tools**: https://github.com/esphome/esp-web-tools/issues

---

**Happy Deploying! 🚀**
