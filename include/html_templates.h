#ifndef HTML_TEMPLATES_H
#define HTML_TEMPLATES_H

#include <Arduino.h>

// ============================================================================
// PORTAL HTML - Initial Setup Configuration Page
// ============================================================================
// Minimal, compact HTML for initial WiFi configuration
// Used during AP mode for first-time setup
const char PORTAL_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Setup</title><style>body{font-family:Arial;margin:20px}input,select{width:100%;padding:8px;margin:5px 0}
button{background:#4CAF50;color:#fff;padding:12px;border:none;width:100%;margin-top:15px}</style></head>
<body><h2>DataTracker Setup</h2><label>WiFi:</label><select id="ssid"></select>
<label>Password:</label><input type="password" id="pwd"><label>Module:</label>
<select id="mod"><option value="bitcoin">Crypto</option><option value="stock">Stock</option><option value="weather">Weather</option></select>
<div id="cfg"></div><button onclick="save()">Complete Step 3/3</button><script>
function save(){var c={ssid:document.getElementById('ssid').value,password:document.getElementById('pwd').value,
module:document.getElementById('mod').value};fetch('/save',{method:'POST',body:JSON.stringify(c)}).then(()=>alert('Saved!'));}
fetch('/scan').then(r=>r.json()).then(n=>{var s=document.getElementById('ssid');n.forEach(x=>s.innerHTML+=
'<option value="'+x.ssid+'">'+x.ssid+'</option>')});
</script></body></html>)rawliteral";

#endif // HTML_TEMPLATES_H
