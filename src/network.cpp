#include "network.h"
#include "config.h"
#include "security.h"
#include "scheduler.h"
#include "version.h"
#include <ESPmDNS.h>
#include <LittleFS.h>

// External objects (initialized in main)
extern SecurityManager security;
extern Scheduler scheduler;

// Debug: Store last POST body and save result for debugging
String lastPostBody = "";
String lastSaveResult = "No save yet";

// Animal names for unique device identification (64 names)
const char* ANIMAL_NAMES[] = {
    "Panda", "Tiger", "Lion", "Bear",
    "Fox", "Wolf", "Cat", "Dog",
    "Bunny", "Otter", "Seal", "Koala",
    "Sloth", "Lemur", "Meerkat", "Badger",
    "Eagle", "Owl", "Hawk", "Falcon",
    "Parrot", "Penguin", "Toucan", "Peacock",
    "Dolphin", "Whale", "Shark", "Turtle",
    "Octopus", "Crab", "Lobster", "Starfish",
    "Giraffe", "Zebra", "Rhino", "Hippo",
    "Elephant", "Monkey", "Gorilla", "Chimp",
    "Hamster", "Squirrel", "Chipmunk", "Hedgehog",
    "Mouse", "Rabbit", "Ferret", "Gecko",
    "Dragon", "Phoenix", "Unicorn", "Griffin",
    "Sphinx", "Kraken", "Hydra", "Basilisk",
    "Cheetah", "Leopard", "Jaguar", "Cougar",
    "Lynx", "Panther", "Puma", "Ocelot"
};

// Minimal HTML for configuration (ultra-compact)
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

// Dynamic Settings page HTML (login + module management)
const char SETTINGS_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>DataTracker Settings</title>
    <style>
        * { box-sizing: border-box; }
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif; margin: 0; padding: 20px; background: #f5f5f5; }
        .container { max-width: 600px; margin: 0 auto; background: #fff; border-radius: 12px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }
        .header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 30px; border-radius: 12px 12px 0 0; text-align: center; }
        .header h1 { margin: 0; font-size: 28px; }
        .header p { margin: 10px 0 0; opacity: 0.9; }
        .content { padding: 30px; }
        .hidden { display: none; }
        .code-input { font-size: 32px; text-align: center; letter-spacing: 15px; width: 100%; padding: 15px; border: 2px solid #ddd; border-radius: 8px; }
        button { background: #667eea; color: white; border: none; padding: 12px 24px; border-radius: 8px; cursor: pointer; font-size: 16px; width: 100%; margin-top: 15px; transition: all 0.3s; }
        button:hover { background: #5568d3; transform: translateY(-1px); }
        button.secondary { background: #6c757d; }
        button.danger { background: #dc3545; }
        button.small { width: auto; padding: 8px 16px; font-size: 14px; }
        .module-list { margin-top: 20px; }
        .module-item { background: #f8f9fa; border: 2px solid #e9ecef; border-radius: 8px; padding: 15px; margin-bottom: 10px; display: flex; align-items: center; cursor: move; transition: all 0.3s; }
        .module-item:hover { border-color: #667eea; box-shadow: 0 2px 8px rgba(102,126,234,0.2); }
        .module-item.dragging { opacity: 0.5; }
        .module-icon { font-size: 32px; margin-right: 15px; }
        .module-info { flex: 1; }
        .module-name { font-weight: bold; font-size: 16px; margin: 0; }
        .module-detail { color: #6c757d; font-size: 14px; margin: 5px 0 0; }
        .module-actions { display: flex; gap: 8px; }
        .module-actions button { width: auto; margin: 0; padding: 6px 12px; font-size: 13px; }
        .add-module { margin-top: 20px; padding: 20px; background: #e7f3ff; border-radius: 8px; border: 2px dashed #667eea; }
        .module-type-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); gap: 10px; margin-top: 15px; }
        .module-type { background: white; border: 2px solid #e9ecef; border-radius: 8px; padding: 15px; text-align: center; cursor: pointer; transition: all 0.3s; }
        .module-type:hover { border-color: #667eea; box-shadow: 0 2px 8px rgba(102,126,234,0.2); }
        .module-type .icon { font-size: 32px; margin-bottom: 8px; }
        .module-type .name { font-size: 14px; font-weight: 500; }
        .modal { display: none; position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.5); z-index: 1000; }
        .modal.active { display: flex; align-items: center; justify-content: center; }
        .modal-content { background: white; border-radius: 12px; padding: 30px; max-width: 500px; width: 90%; max-height: 80vh; overflow-y: auto; }
        .modal-header { font-size: 24px; font-weight: bold; margin-bottom: 20px; }
        .form-group { margin-bottom: 20px; }
        .form-group label { display: block; font-weight: 500; margin-bottom: 8px; }
        .form-group input, .form-group select { width: 100%; padding: 10px; border: 2px solid #e9ecef; border-radius: 8px; font-size: 16px; }
        .form-actions { display: flex; gap: 10px; margin-top: 20px; }
        .form-actions button { flex: 1; }
        .message { padding: 12px; border-radius: 8px; margin-bottom: 20px; font-weight: 500; }
        .message.error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
        .message.success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .search-container { position: relative; }
        .search-results { border: 1px solid #ddd; border-radius: 8px; margin-top: 5px; max-height: 200px; overflow-y: auto; background: white; }
        .search-item { padding: 10px; cursor: pointer; border-bottom: 1px solid #eee; display: flex; align-items: center; }
        .search-item:hover { background: #f5f5f5; }
        .search-item:last-child { border-bottom: none; }
        .search-item img { width: 24px; height: 24px; margin-right: 10px; border-radius: 50%; }
        .text-center { text-align: center; }
        .mt-20 { margin-top: 20px; }
        .flex-end { display: flex; justify-content: flex-end; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>DataTracker</h1>
            <p id="header-subtitle">Settings & Configuration</p>
        </div>
        <div id="login-view" class="content">
            <p class="text-center">Enter code from device display:</p>
            <input type="text" id="code" class="code-input" maxlength="6" placeholder="000000" pattern="[0-9]{6}" autofocus onkeypress="if(event.key==='Enter')validateCode()">
            <button onclick="validateCode()">Unlock Settings</button>
            <div id="login-error" class="message error hidden"></div>
        </div>
        <div id="settings-view" class="content hidden">
            <div class="flex-end" style="margin-bottom: 20px;">
                <button class="small secondary" onclick="logout()">Logout</button>
            </div>
            <div id="message" class="message hidden"></div>
            <h3>Active Modules</h3>
            <p style="color: #6c757d; font-size: 14px;">Drag to reorder - Click Edit to configure</p>
            <div id="module-list" class="module-list"></div>
            <div class="add-module">
                <h4 style="margin-top: 0;">Add New Module</h4>
                <p style="color: #6c757d; font-size: 14px; margin-bottom: 15px;">Choose a module type to add:</p>
                <div id="module-types" class="module-type-grid"></div>
            </div>
            <div class="mt-20">
                <h3>Device Settings</h3>
                <div class="form-group">
                    <label>Currency:</label>
                    <select id="currency">
                        <option value="USD">US Dollar (USD)</option>
                        <option value="EUR">Euro (EUR)</option>
                        <option value="GBP">British Pound (GBP)</option>
                        <option value="JPY">Japanese Yen (JPY)</option>
                        <option value="CAD">Canadian Dollar (CAD)</option>
                        <option value="AUD">Australian Dollar (AUD)</option>
                        <option value="CHF">Swiss Franc (CHF)</option>
                        <option value="CNY">Chinese Yuan (CNY)</option>
                        <option value="INR">Indian Rupee (INR)</option>
                        <option value="BRL">Brazilian Real (BRL)</option>
                    </select>
                </div>
                <div class="form-group">
                    <label>Thousand Separator:</label>
                    <select id="thousandSep">
                        <option value="">None (1000) ⭐ Recommended</option>
                        <option value="'">Apostrophe (1'000) - Tight</option>
                        <option value=",">Comma (1,000)</option>
                        <option value=".">Period (1.000)</option>
                        <option value=" ">Space (1 000)</option>
                    </select>
                </div>
                <button onclick="saveDeviceSettings()">Save Settings</button>
            </div>
            <div class="mt-20">
                <button class="danger" onclick="restartDevice()">Restart Device</button>
                <button class="danger" onclick="factoryReset()">Factory Reset</button>
            </div>
        </div>
    </div>
    <div id="module-modal" class="modal">
        <div class="modal-content">
            <div class="modal-header" id="modal-title">Add Module</div>
            <div id="modal-form"></div>
            <div class="form-actions">
                <button class="secondary" onclick="closeModal()">Cancel</button>
                <button id="modal-save-btn" onclick="saveModule()">Save</button>
            </div>
        </div>
    </div>
    <script>
        let token = '';
        let modules = [];
        let moduleTypes = [];
        let currentModule = null;
        let searchTimeout = null;
        let draggedItem = null;
        function validateCode() {
            const code = document.getElementById('code').value;
            if (code.length !== 6) {
                showLoginError('Enter 6-digit code');
                return;
            }
            fetch('/api/validate', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({code: parseInt(code)})
            })
            .then(r => r.json())
            .then(d => {
                if (d.valid) {
                    token = d.token;
                    showSettings();
                } else {
                    showLoginError(d.error || 'Invalid code');
                }
            })
            .catch(() => showLoginError('Connection error'));
        }
        function showLoginError(msg) {
            const el = document.getElementById('login-error');
            el.textContent = msg;
            el.classList.remove('hidden');
        }
        function showSettings() {
            document.getElementById('login-view').classList.add('hidden');
            document.getElementById('settings-view').classList.remove('hidden');
            loadSettings();
        }
        function logout() {
            token = '';
            document.getElementById('settings-view').classList.add('hidden');
            document.getElementById('login-view').classList.remove('hidden');
            document.getElementById('code').value = '';
        }
        function loadSettings() {
            loadModules();
            loadModuleTypes();
            loadConfig();
        }
        function loadModules() {
            fetch('/api/modules', {headers: {'Authorization': token}})
            .then(r => {
                if (r.status === 401) { logout(); return null; }
                if (!r.ok) {
                    return r.text().then(text => {
                        throw new Error(`HTTP ${r.status}: ${text}`);
                    });
                }
                return r.json();
            })
            .then(d => {
                if (!d) return;
                modules = d.modules || [];
                renderModules();
            })
            .catch(e => {
                console.error('Load modules error:', e);
                showMessage('Failed to load modules: ' + e.message, 'error');
            });
        }
        // Built-in Transit Agency Database
        const builtInAgencies = [
            {
                id: "prague-pid",
                name: "Prague Public Transit (PID)",
                searchTerms: ["prague", "praha", "pid", "czech", "czechia"],
                region: "Prague, Czech Republic",
                country: "CZ",
                icon: "🚊",
                apiType: "golemio",
                baseUrl: "https://api.golemio.cz/v2",
                requiresApiKey: true,
                apiKeyHelp: "Get free API key at https://api.golemio.cz",
                stopSearchPattern: "/gtfs/stops?latlng={lat},{lon}&range=500",
                jsonPaths: {
                    stopId: "features[0].properties.stop_id",
                    stopName: "features[0].properties.stop_name",
                    arrivalTime: "departures[0].departure_timestamp.predicted"
                }
            },
            {
                id: "sf-bart",
                name: "BART",
                searchTerms: ["bart", "san francisco", "bay area", "oakland", "sf"],
                region: "San Francisco Bay Area, CA",
                country: "USA",
                icon: "🚇",
                apiType: "bart-xml",
                baseUrl: "https://api.bart.gov/api",
                requiresApiKey: true,
                apiKeyHelp: "Get free API key at https://api.bart.gov/api/",
                stopSearchPattern: "/stn.aspx?cmd=stns&key={apiKey}&json=y",
                jsonPaths: {
                    arrivalTime: "root.etd[0].estimate[0].minutes"
                }
            },
            {
                id: "nyc-mta",
                name: "NYC Subway (MTA)",
                searchTerms: ["nyc", "new york", "mta", "subway", "manhattan"],
                region: "New York, NY",
                country: "USA",
                icon: "🚇",
                apiType: "transitland",
                transitlandId: "o-dr5r-nyct",
                baseUrl: "https://transit.land/api/v2/rest",
                requiresApiKey: false,
                jsonPaths: {
                    arrivalTime: "schedule_stop_pairs[0].origin_departure_time"
                }
            },
            {
                id: "la-metro",
                name: "LA Metro",
                searchTerms: ["la", "los angeles", "metro", "california"],
                region: "Los Angeles, CA",
                country: "USA",
                icon: "🚇",
                apiType: "transitland",
                transitlandId: "o-9q5-metro",
                baseUrl: "https://transit.land/api/v2/rest",
                requiresApiKey: false,
                jsonPaths: {
                    arrivalTime: "schedule_stop_pairs[0].origin_departure_time"
                }
            },
            {
                id: "london-tfl",
                name: "Transport for London (TfL)",
                searchTerms: ["london", "tfl", "tube", "underground", "uk"],
                region: "London, United Kingdom",
                country: "UK",
                icon: "🚇",
                apiType: "tfl",
                baseUrl: "https://api.tfl.gov.uk",
                requiresApiKey: false,
                stopSearchPattern: "/StopPoint/Search/{query}",
                jsonPaths: {
                    arrivalTime: "lineStatuses[0].statusSeverityDescription"
                }
            }
        ];

        // Selected agency and stop tracking
        let selectedAgency = null;
        let selectedStop = null;
        let selectedRoute = null;
        let wizardStep = 1;

        // Geocoding helper using Nominatim (OpenStreetMap)
        async function geocodeAddress(address) {
            const url = `https://nominatim.openstreetmap.org/search?q=${encodeURIComponent(address)}&format=json&limit=1`;
            const response = await fetch(url, {
                headers: {'User-Agent': 'DataTracker-Transit'}
            });
            const data = await response.json();
            if (data.length > 0) {
                return {lat: parseFloat(data[0].lat), lon: parseFloat(data[0].lon)};
            }
            throw new Error('Location not found');
        }

        // Search agencies by keyword
        function searchAgencies(query) {
            if (!builtInAgencies) return [];
            if (!query || query.length < 2) return builtInAgencies;
            query = query.toLowerCase();
            return builtInAgencies.filter(a =>
                a.searchTerms.some(term => term.includes(query)) ||
                a.name.toLowerCase().includes(query) ||
                a.region.toLowerCase().includes(query)
            );
        }

        // Search stops for Prague PID (Golemio API)
        async function searchPragueStops(address, apiKey) {
            const coords = await geocodeAddress(address + ', Prague, Czech Republic');
            const url = `https://api.golemio.cz/v2/gtfs/stops?latlng=${coords.lat},${coords.lon}&range=500`;
            const response = await fetch(url, {
                headers: {
                    'X-Access-Token': apiKey,
                    'Content-Type': 'application/json'
                }
            });
            const data = await response.json();
            return data.features.map(f => ({
                id: f.properties.stop_id,
                name: f.properties.stop_name,
                coords: f.geometry.coordinates
            }));
        }

        // Search stops using TransitLand API
        async function searchTransitLandStops(address, operatorId) {
            const coords = await geocodeAddress(address);
            const url = `https://transit.land/api/v2/rest/stops?lat=${coords.lat}&lon=${coords.lon}&radius=500&served_by_onestop_ids=${operatorId}`;
            const response = await fetch(url);
            const data = await response.json();
            return data.stops.map(s => ({
                id: s.onestop_id,
                name: s.stop_name,
                stopCode: s.stop_id
            }));
        }

        // Wizard navigation functions
        function filterAgencies(query) {
            const list = document.getElementById('agency-list');
            if (!list) return;

            const results = searchAgencies(query);
            if (!results) return;

            list.innerHTML = results.map(a => `
                <div onclick="selectAgency('${a.id}')" style="padding:12px; margin-bottom:8px; border:1px solid #ddd; border-radius:8px; cursor:pointer; transition:all 0.2s;"
                     onmouseover="this.style.borderColor='#667eea'; this.style.background='#f8f9ff';"
                     onmouseout="this.style.borderColor='#ddd'; this.style.background='white';">
                    <div style="font-size:24px; margin-bottom:4px;">${a.icon}</div>
                    <div style="font-weight:600; font-size:14px;">${a.name}</div>
                    <div style="color:#666; font-size:12px;">${a.region}</div>
                    ${a.requiresApiKey ? '<div style="color:#ff9800; font-size:11px; margin-top:4px;">🔑 API key required</div>' : '<div style="color:#4caf50; font-size:11px; margin-top:4px;">✓ No API key needed</div>'}
                </div>
            `).join('');
        }

        function selectAgency(agencyId) {
            selectedAgency = builtInAgencies.find(a => a.id === agencyId);
            if (!selectedAgency) return;

            // Show step 2
            document.getElementById('step-agency').style.display = 'none';
            document.getElementById('step-stop').style.display = 'block';

            // Show API key input if needed
            if (selectedAgency.requiresApiKey) {
                document.getElementById('api-key-input').style.display = 'block';
                document.getElementById('api-key-help').textContent = selectedAgency.apiKeyHelp || '';
            }
            wizardStep = 2;
        }

        async function doStopSearch() {
            const address = document.getElementById('stop-search').value;
            if (!address) {
                showMessage('Please enter an address', 'error');
                return;
            }

            const resultsDiv = document.getElementById('stop-results');
            resultsDiv.innerHTML = '<p>Searching...</p>';

            try {
                let stops = [];
                if (selectedAgency.apiType === 'golemio') {
                    const apiKey = document.getElementById('transit-apikey').value;
                    if (!apiKey) {
                        showMessage('API key required for Prague PID', 'error');
                        return;
                    }
                    stops = await searchPragueStops(address, apiKey);
                } else if (selectedAgency.apiType === 'transitland') {
                    stops = await searchTransitLandStops(address, selectedAgency.transitlandId);
                }

                if (stops.length === 0) {
                    resultsDiv.innerHTML = '<p style="color:#888;">No stops found. Try a different address.</p>';
                    return;
                }

                resultsDiv.innerHTML = stops.map(s => `
                    <div onclick="selectStop('${s.id}', '${s.name}')" style="padding:10px; margin-bottom:6px; border:1px solid #ddd; border-radius:6px; cursor:pointer;"
                         onmouseover="this.style.background='#f0f0f0';" onmouseout="this.style.background='white';">
                        <div style="font-weight:500;">📍 ${s.name}</div>
                        <div style="color:#888; font-size:11px;">Stop ID: ${s.stopCode || s.id}</div>
                    </div>
                `).join('');
            } catch (error) {
                resultsDiv.innerHTML = `<p style="color:#d32f2f;">Error: ${error.message}</p>`;
            }
        }

        function selectStop(stopId, stopName) {
            selectedStop = {id: stopId, name: stopName};

            // Move to review step
            document.getElementById('step-stop').style.display = 'none';
            document.getElementById('step-review').style.display = 'block';

            // Show summary
            const summary = document.getElementById('review-summary');
            summary.innerHTML = `
                <div style="margin-bottom:8px;"><strong>Agency:</strong> ${selectedAgency.icon} ${selectedAgency.name}</div>
                <div style="margin-bottom:8px;"><strong>Stop:</strong> ${selectedStop.name}</div>
                <div style="color:#666; font-size:12px;">Stop ID: ${selectedStop.id}</div>
            `;
            wizardStep = 3;
        }

        function wizardBack() {
            if (wizardStep === 2) {
                document.getElementById('step-stop').style.display = 'none';
                document.getElementById('step-agency').style.display = 'block';
                wizardStep = 1;
            } else if (wizardStep === 3) {
                document.getElementById('step-review').style.display = 'none';
                document.getElementById('step-stop').style.display = 'block';
                wizardStep = 2;
            }
        }

        function useManualSetup() {
            document.getElementById('step-agency').style.display = 'none';
            document.getElementById('step-manual').style.display = 'block';
        }

        function saveTransitWizard() {
            // Build API URL based on agency type
            let apiUrl = '';
            const apiKey = document.getElementById('transit-apikey')?.value || '';

            if (selectedAgency.apiType === 'golemio') {
                // Prague PID - use Golemio departures API
                apiUrl = `${selectedAgency.baseUrl}/gtfs/stoptimes?stop_id=${selectedStop.id}&limit=1&order=departure_timestamp`;
            } else if (selectedAgency.apiType === 'transitland') {
                // TransitLand real-time API
                apiUrl = `${selectedAgency.baseUrl}/schedule_stop_pairs?origin_onestop_id=${selectedStop.id}&active=true`;
            }

            // Populate hidden form fields (create them if in wizard mode)
            if (!document.getElementById('routeName')) {
                const form = document.getElementById('modal-form');
                form.innerHTML += `
                    <input type="hidden" id="routeName" value="">
                    <input type="hidden" id="stopName" value="">
                    <input type="hidden" id="apiUrl" value="">
                    <input type="hidden" id="jsonPath" value="">
                    <input type="hidden" id="apiKey" value="">
                `;
            }

            document.getElementById('routeName').value = selectedStop.name.split(/[-–]/)[0].trim();
            document.getElementById('stopName').value = selectedStop.name;
            document.getElementById('apiUrl').value = apiUrl;
            document.getElementById('jsonPath').value = selectedAgency.jsonPaths?.arrivalTime || '';
            document.getElementById('apiKey').value = apiKey;

            // Call existing saveModule function
            saveModule();
        }

        function loadModuleTypes() {
            fetch('/api/module-types', {headers: {'Authorization': token}})
            .then(r => r.json())
            .then(d => {
                moduleTypes = d.types || [];
                renderModuleTypes();
            });
        }
        function loadConfig() {
            fetch('/api/config', {headers: {'Authorization': token}})
            .then(r => r.json())
            .then(d => {
                document.getElementById('currency').value = d.device.currency || 'USD';
                document.getElementById('thousandSep').value = d.device.thousandSep !== undefined ? d.device.thousandSep : ',';
            });
        }
        function renderModules() {
            const list = document.getElementById('module-list');
            if (modules.length === 0) {
                list.innerHTML = '<p style="text-align:center;color:#6c757d;padding:20px;">No modules yet. Add one below!</p>';
                return;
            }
            list.innerHTML = modules.map((m, idx) => {
                const icon = getModuleIcon(m.type);
                const name = getModuleName(m);
                const detail = getModuleDetail(m);
                return `<div class="module-item" draggable="true" data-id="${m.id}">
                    <div class="module-icon">${icon}</div>
                    <div class="module-info">
                        <div class="module-name">${name}</div>
                        <div class="module-detail">${detail}</div>
                    </div>
                    <div class="module-actions">
                        <button class="small" onclick="editModule('${m.id}')">Edit</button>
                        <button class="small danger" onclick="deleteModule('${m.id}')">Delete</button>
                    </div>
                </div>`;
            }).join('');
            setupDragDrop();
        }
        function getModuleIcon(type) {
            const icons = {crypto: 'B', stock: 'S', weather: 'W', custom: 'C', settings: 'S', countdown: '⏰', http: '🌐'};
            return icons[type] || 'M';
        }
        function getModuleName(m) {
            if (m.type === 'crypto') return m.cryptoName || m.cryptoSymbol || 'Crypto';
            if (m.type === 'stock') return m.name || m.ticker || 'Stock';
            if (m.type === 'weather') return m.location || 'Weather';
            if (m.type === 'custom') return m.label || 'Custom';
            if (m.type === 'countdown') return m.label || 'Countdown';
            if (m.type === 'http') return m.label || 'HTTP Fetch';
            if (m.type === 'settings') return 'Settings';
            return m.id;
        }
        function getModuleDetail(m) {
            if (m.type === 'crypto') return `${m.cryptoSymbol} - $${(m.value || 0).toFixed(2)}`;
            if (m.type === 'stock') return `${m.ticker} - $${(m.value || 0).toFixed(2)}`;
            if (m.type === 'weather') return `${(m.temperature || 0).toFixed(1)}C - ${m.condition || 'Unknown'}`;
            if (m.type === 'custom') return `${(m.value || 0).toFixed(2)} ${m.unit || ''}`;
            if (m.type === 'countdown') {
                const targetDate = m.targetDate || 'Not set';
                const type = m.countdownType || 'date';
                if (type === 'datetime') {
                    return `${targetDate} ${m.targetTime || '00:00'}`;
                }
                return targetDate;
            }
            if (m.type === 'http') return m.url ? new URL(m.url).hostname : 'No URL set';
            return 'Configuration module';
        }
        function setupDragDrop() {
            const items = document.querySelectorAll('.module-item');
            items.forEach(item => {
                item.addEventListener('dragstart', handleDragStart);
                item.addEventListener('dragover', handleDragOver);
                item.addEventListener('drop', handleDrop);
                item.addEventListener('dragend', handleDragEnd);
            });
        }
        function handleDragStart(e) {
            draggedItem = this;
            this.classList.add('dragging');
            e.dataTransfer.effectAllowed = 'move';
        }
        function handleDragOver(e) {
            e.preventDefault();
            e.dataTransfer.dropEffect = 'move';
            const afterElement = getDragAfterElement(this.parentNode, e.clientY);
            if (afterElement == null) {
                this.parentNode.appendChild(draggedItem);
            } else {
                this.parentNode.insertBefore(draggedItem, afterElement);
            }
        }
        function handleDrop(e) {
            e.stopPropagation();
            return false;
        }
        function handleDragEnd(e) {
            this.classList.remove('dragging');
            saveModuleOrder();
        }
        function getDragAfterElement(container, y) {
            const draggableElements = [...container.querySelectorAll('.module-item:not(.dragging)')];
            return draggableElements.reduce((closest, child) => {
                const box = child.getBoundingClientRect();
                const offset = y - box.top - box.height / 2;
                if (offset < 0 && offset > closest.offset) {
                    return {offset: offset, element: child};
                } else {
                    return closest;
                }
            }, {offset: Number.NEGATIVE_INFINITY}).element;
        }
        function saveModuleOrder() {
            const items = document.querySelectorAll('.module-item');
            const order = Array.from(items).map(item => item.getAttribute('data-id'));
            fetch('/api/modules/order', {
                method: 'POST',
                headers: {'Authorization': token, 'Content-Type': 'application/json'},
                body: JSON.stringify({order: order})
            })
            .then(r => r.json())
            .then(d => {
                if (d.success) {
                    showMessage('Module order updated', 'success');
                }
            })
            .catch(e => showMessage('Failed to update order', 'error'));
        }
        function renderModuleTypes() {
            const grid = document.getElementById('module-types');
            grid.innerHTML = moduleTypes.map(t => `
                <div class="module-type" onclick="openAddModule('${t.id}')">
                    <div class="icon">${t.icon}</div>
                    <div class="name">${t.name}</div>
                </div>
            `).join('');
        }
        function openAddModule(type) {
            currentModule = {type: type, id: generateModuleId(type)};
            document.getElementById('modal-title').textContent = 'Add ' + moduleTypes.find(t => t.id === type).name;
            renderModuleForm(type);
            document.getElementById('module-modal').classList.add('active');
        }
        function editModule(id) {
            currentModule = modules.find(m => m.id === id);
            if (!currentModule) return;
            const typeName = moduleTypes.find(t => t.id === currentModule.type)?.name || 'Module';
            document.getElementById('modal-title').textContent = 'Edit ' + typeName;
            renderModuleForm(currentModule.type, currentModule);
            document.getElementById('module-modal').classList.add('active');
        }
        function generateModuleId(type) {
            const existing = modules.filter(m => m.type === type).length;
            return type + '_' + Date.now();
        }
        function getRefreshIntervalHTML(defaultValue = 300, recommended = '5 min (default)') {
            return `
                <div class="form-group">
                    <label>Refresh Interval:</label>
                    <input type="range" id="refreshInterval" min="10" max="3600" value="${defaultValue}"
                           oninput="document.getElementById('refresh-value').textContent = formatInterval(this.value)">
                    <div style="display: flex; justify-content: space-between; align-items: center; margin-top: 4px;">
                        <span id="refresh-value" style="font-weight: bold;">${formatInterval(defaultValue)}</span>
                        <small style="color: #888;">Recommended: ${recommended}</small>
                    </div>
                    <small style="color: #888; font-size: 11px;">⚠️ Intervals <60s may hit API rate limits</small>
                </div>
            `;
        }
        function formatInterval(seconds) {
            seconds = parseInt(seconds);
            if (seconds < 60) return seconds + 's';
            if (seconds < 3600) return Math.floor(seconds / 60) + 'm';
            return Math.floor(seconds / 3600) + 'h';
        }
        function renderModuleForm(type, data = {}) {
            const form = document.getElementById('modal-form');
            if (type === 'crypto') {
                const refreshInterval = data.refreshInterval || 300;
                form.innerHTML = `
                    <div class="form-group">
                        <label>Search Cryptocurrency:</label>
                        <div class="search-container">
                            <input type="text" id="crypto-search" placeholder="Search..." oninput="searchCrypto(this.value)">
                            <div id="crypto-results" class="search-results" style="display:none"></div>
                        </div>
                    </div>
                    <div class="form-group">
                        <label>Crypto ID:</label>
                        <input type="text" id="cryptoId" value="${data.cryptoId || ''}" readonly>
                    </div>
                    <div class="form-group">
                        <label>Symbol:</label>
                        <input type="text" id="cryptoSymbol" value="${data.cryptoSymbol || ''}" readonly>
                    </div>
                    <div class="form-group">
                        <label>Name:</label>
                        <input type="text" id="cryptoName" value="${data.cryptoName || ''}" readonly>
                    </div>
                    <div class="form-group">
                        <label>Decimal Places:</label>
                        <input type="number" id="decimals" value="${data.decimals !== undefined ? data.decimals : 'auto'}" min="0" max="8" placeholder="auto">
                        <small style="color: #888; font-size: 11px;">Leave blank for auto (BTC: 0, ETH: 0, <$1: 4-6)</small>
                    </div>
                    ${getRefreshIntervalHTML(refreshInterval, '5 min (default) or 1 min for trading')}
                `;
            } else if (type === 'stock') {
                const refreshInterval = data.refreshInterval || 300;
                form.innerHTML = `
                    <div class="form-group">
                        <label>Search Stock:</label>
                        <div class="search-container">
                            <input type="text" id="stock-search" placeholder="Search ticker..." oninput="searchStock(this.value)">
                            <div id="stock-results" class="search-results" style="display:none"></div>
                        </div>
                    </div>
                    <div class="form-group">
                        <label>Ticker Symbol:</label>
                        <input type="text" id="ticker" value="${data.ticker || ''}" placeholder="AAPL">
                    </div>
                    <div class="form-group">
                        <label>Company Name:</label>
                        <input type="text" id="stockName" value="${data.name || ''}" placeholder="Apple Inc.">
                    </div>
                    <div class="form-group">
                        <label>Decimal Places:</label>
                        <input type="number" id="decimals" value="${data.decimals !== undefined ? data.decimals : 'auto'}" min="0" max="8" placeholder="auto">
                        <small style="color: #888; font-size: 11px;">Leave blank for auto (>$100: 0, else: 2)</small>
                    </div>
                    ${getRefreshIntervalHTML(refreshInterval, '5 min (default) or 1 min for day trading')}
                `;
            } else if (type === 'weather') {
                const refreshInterval = data.refreshInterval || 900;
                form.innerHTML = `
                    <div class="form-group">
                        <label>Search Location:</label>
                        <input type="text" id="weather-search" placeholder="Start typing city name..." oninput="searchWeather(this.value)" autocomplete="off">
                        <div id="weather-results" style="display:none; max-height:150px; overflow-y:auto; border:1px solid #ddd; margin-top:4px;"></div>
                    </div>
                    <div class="form-group">
                        <label>Selected Location:</label>
                        <input type="text" id="location" value="${data.location || ''}" readonly style="background:#f5f5f5">
                    </div>
                    <input type="hidden" id="latitude" value="${data.latitude || 0}">
                    <input type="hidden" id="longitude" value="${data.longitude || 0}">
                    ${getRefreshIntervalHTML(refreshInterval, '15 min (default) or 5 min for severe weather')}
                `;
            } else if (type === 'custom') {
                form.innerHTML = `
                    <div class="form-group">
                        <label>Label:</label>
                        <input type="text" id="label" value="${data.label || 'My Metric'}" maxlength="20">
                    </div>
                    <div class="form-group">
                        <label>Value:</label>
                        <input type="number" step="0.01" id="value" value="${data.value || 0}">
                    </div>
                    <div class="form-group">
                        <label>Unit:</label>
                        <input type="text" id="unit" value="${data.unit || ''}" placeholder="units" maxlength="10">
                    </div>
                    <div class="form-group">
                        <label>Decimal Places:</label>
                        <select id="decimals">
                            <option value="-1" ${(data.decimals === -1 || data.decimals === undefined) ? 'selected' : ''}>Auto</option>
                            <option value="0" ${data.decimals === 0 ? 'selected' : ''}>0 (e.g., 123)</option>
                            <option value="1" ${data.decimals === 1 ? 'selected' : ''}>1 (e.g., 123.4)</option>
                            <option value="2" ${data.decimals === 2 ? 'selected' : ''}>2 (e.g., 123.45)</option>
                            <option value="3" ${data.decimals === 3 ? 'selected' : ''}>3 (e.g., 123.456)</option>
                        </select>
                    </div>
                `;
            } else if (type === 'countdown') {
                const refreshInterval = data.refreshInterval || 3600;

                form.innerHTML = `
                    <div class="form-group">
                        <label>Event Name:</label>
                        <input type="text" id="label" value="${data.label || 'Event'}" maxlength="20">
                    </div>
                    <div class="form-group">
                        <label>Countdown Type:</label>
                        <select id="countdownType" onchange="toggleCountdownType(this.value)">
                            <option value="date" ${(data.countdownType || 'date') === 'date' ? 'selected' : ''}>Date Only (days)</option>
                            <option value="datetime" ${data.countdownType === 'datetime' ? 'selected' : ''}>Date & Time (hours)</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label>Target Date:</label>
                        <input type="date" id="targetDate" value="${data.targetDate || ''}">
                    </div>
                    <div class="form-group" id="time-field" style="${(data.countdownType === 'datetime') ? '' : 'display:none'}">
                        <label>Target Time:</label>
                        <input type="time" id="targetTime" value="${data.targetTime || '00:00'}">
                    </div>
                    <div class="form-group">
                        <label>Decimal Places:</label>
                        <select id="decimals">
                            <option value="-1" ${(data.decimals === -1 || data.decimals === undefined) ? 'selected' : ''}>Auto</option>
                            <option value="0" ${data.decimals === 0 ? 'selected' : ''}>0 (e.g., 5 days)</option>
                            <option value="1" ${data.decimals === 1 ? 'selected' : ''}>1 (e.g., 5.5 days)</option>
                            <option value="2" ${data.decimals === 2 ? 'selected' : ''}>2 (e.g., 5.25 days)</option>
                        </select>
                    </div>
                    ${getRefreshIntervalHTML(refreshInterval, '1 hour (auto-update)')}
                `;
            } else if (type === 'http') {
                const refreshInterval = data.refreshInterval || 300;

                form.innerHTML = `
                    <div class="form-group">
                        <label>Label:</label>
                        <input type="text" id="label" value="${data.label || 'API Data'}" maxlength="20">
                    </div>
                    <div class="form-group">
                        <label>API URL:</label>
                        <input type="text" id="url" value="${data.url || ''}" placeholder="https://api.example.com/data">
                        <small style="color: #888; font-size: 11px;">Full URL to API endpoint</small>
                    </div>
                    <div class="form-group">
                        <label>JSON Path (optional):</label>
                        <input type="text" id="jsonPath" value="${data.jsonPath || ''}" placeholder="data.temperature">
                        <small style="color: #888; font-size: 11px;">Examples: "value", "data.temp", "items[0].count"</small>
                    </div>
                    <div class="form-group">
                        <label>Unit:</label>
                        <input type="text" id="unit" value="${data.unit || ''}" placeholder="units" maxlength="10">
                    </div>
                    ${getRefreshIntervalHTML(refreshInterval, '5 minutes')}
                `;
            } else if (type === 'transit') {
                const refreshInterval = data.refreshInterval || 30;
                wizardStep = 1;
                selectedAgency = null;
                selectedStop = null;

                form.innerHTML = `
                    <!-- Transit Setup Wizard -->
                    <div id="transit-wizard">
                        <!-- Step 1: Agency Selection -->
                        <div id="step-agency" class="wizard-step">
                            <h4>Step 1: Select Transit Agency</h4>
                            <input type="text" id="agency-search" placeholder="Search: Prague, NYC, London..."
                                   oninput="filterAgencies(this.value)" style="width:100%; padding:8px; margin-bottom:12px;">
                            <div id="agency-list" style="max-height:300px; overflow-y:auto;"></div>
                            <div style="margin-top:16px; padding-top:16px; border-top:1px solid #ddd;">
                                <button onclick="useManualSetup()" class="secondary" style="width:100%;">
                                    ⚙️ Manual Setup (Advanced)
                                </button>
                            </div>
                        </div>

                        <!-- Step 2: Stop Search -->
                        <div id="step-stop" class="wizard-step" style="display:none;">
                            <h4>Step 2: Find Your Stop</h4>
                            <p style="color:#666; font-size:13px;">Enter street address or intersection</p>
                            <div style="display:flex; gap:8px; margin-bottom:12px;">
                                <input type="text" id="stop-search" placeholder="e.g., Main St & 5th Ave" style="flex:1; padding:8px;">
                                <button onclick="doStopSearch()" style="padding:8px 16px;">Search</button>
                            </div>
                            <div id="api-key-input" style="display:none; margin-bottom:12px;">
                                <label>API Key:</label>
                                <input type="password" id="transit-apikey" placeholder="Required for this agency" style="width:100%; padding:8px;">
                                <small id="api-key-help" style="color:#888; font-size:11px;"></small>
                            </div>
                            <div id="stop-results" style="max-height:250px; overflow-y:auto;"></div>
                            <button onclick="wizardBack()" style="margin-top:12px;">← Back</button>
                        </div>

                        <!-- Step 3: Review & Save -->
                        <div id="step-review" class="wizard-step" style="display:none;">
                            <h4>Step 3: Review Configuration</h4>
                            <div id="review-summary" style="background:#f5f5f5; padding:12px; border-radius:8px; margin-bottom:16px;"></div>
                            ${getRefreshIntervalHTML(refreshInterval, '30s for real-time transit')}
                            <div style="display:flex; gap:8px; margin-top:16px;">
                                <button onclick="wizardBack()" style="flex:1;">← Back</button>
                                <button onclick="saveTransitWizard()" style="flex:2; background:#4CAF50;">Save Module ✓</button>
                            </div>
                        </div>

                        <!-- Manual Setup Mode -->
                        <div id="step-manual" class="wizard-step" style="display:none;">
                            <h4>Manual Setup</h4>
                            <div class="form-group">
                                <label>Route Name:</label>
                                <input type="text" id="routeName" placeholder="Bus 43, Red Line, etc.">
                            </div>
                            <div class="form-group">
                                <label>Stop Name:</label>
                                <input type="text" id="stopName" placeholder="Main & 5th Ave">
                            </div>
                            <div class="form-group">
                                <label>API URL:</label>
                                <input type="text" id="apiUrl" placeholder="https://api.example.com/arrivals">
                            </div>
                            <div class="form-group">
                                <label>JSON Path:</label>
                                <input type="text" id="jsonPath" placeholder="arrivals[0].minutes">
                            </div>
                            <div class="form-group">
                                <label>API Key (optional):</label>
                                <input type="password" id="apiKey">
                            </div>
                            ${getRefreshIntervalHTML(refreshInterval, '30s default')}
                            <button onclick="wizardBack()">← Back to Wizard</button>
                        </div>
                    </div>
                `;

                // Initialize agency list
                setTimeout(() => filterAgencies(''), 100);
            } else if (type === 'quad') {
                // Build module selector options from current modules
                let moduleOptions = '<option value="">-- None --</option>';
                modules.forEach(m => {
                    if (m.type !== 'quad' && m.type !== 'settings') {
                        const name = getModuleName(m);
                        moduleOptions += `<option value="${m.id}">${name}</option>`;
                    }
                });

                form.innerHTML = `
                    <div class="form-group">
                        <label>Top Left Module:</label>
                        <select id="slot1">${moduleOptions}</select>
                    </div>
                    <div class="form-group">
                        <label>Top Right Module:</label>
                        <select id="slot2">${moduleOptions}</select>
                    </div>
                    <div class="form-group">
                        <label>Bottom Left Module:</label>
                        <select id="slot3">${moduleOptions}</select>
                    </div>
                    <div class="form-group">
                        <label>Bottom Right Module:</label>
                        <select id="slot4">${moduleOptions}</select>
                    </div>
                `;

                // Set current values if editing
                if (data.slot1) document.getElementById('slot1').value = data.slot1;
                if (data.slot2) document.getElementById('slot2').value = data.slot2;
                if (data.slot3) document.getElementById('slot3').value = data.slot3;
                if (data.slot4) document.getElementById('slot4').value = data.slot4;
            }
        }
        function searchCrypto(query) {
            clearTimeout(searchTimeout);
            if (query.length < 2) {
                document.getElementById('crypto-results').style.display = 'none';
                return;
            }
            const results = document.getElementById('crypto-results');
            results.innerHTML = '<div class="search-item">Searching...</div>';
            results.style.display = 'block';
            searchTimeout = setTimeout(() => {
                fetch(`https://api.coingecko.com/api/v3/search?query=${encodeURIComponent(query)}`)
                .then(r => r.json())
                .then(d => {
                    results.innerHTML = d.coins.slice(0,5).map(coin => `
                        <div class="search-item" onclick="selectCrypto('${coin.id}', '${coin.symbol}', '${coin.name.replace(/'/g, "\\'")}')">
                            ${coin.thumb ? `<img src="${coin.thumb}">` : ''}
                            ${coin.name} (${coin.symbol.toUpperCase()})
                        </div>
                    `).join('') || '<div class="search-item">No results</div>';
                })
                .catch(() => results.innerHTML = '<div class="search-item">Error searching</div>');
            }, 300);
        }
        function selectCrypto(id, symbol, name) {
            document.getElementById('cryptoId').value = id;
            document.getElementById('cryptoSymbol').value = symbol.toUpperCase();
            document.getElementById('cryptoName').value = name;
            document.getElementById('crypto-search').value = '';
            document.getElementById('crypto-results').style.display = 'none';
        }
        function searchStock(query) {
            clearTimeout(searchTimeout);
            if (query.length < 1) {
                document.getElementById('stock-results').style.display = 'none';
                return;
            }
            const results = document.getElementById('stock-results');
            results.innerHTML = '<div class="search-item">Searching...</div>';
            results.style.display = 'block';
            searchTimeout = setTimeout(() => {
                fetch(`/api/stock-search?q=${encodeURIComponent(query)}`)
                .then(r => r.json())
                .then(d => {
                    if (d.quotes && d.quotes.length > 0) {
                        results.innerHTML = d.quotes.filter(q => q.quoteType === 'EQUITY' || q.quoteType === 'ETF').slice(0,5).map(q => `
                            <div class="search-item" onclick="selectStock('${q.symbol}', '${(q.shortname || q.longname || q.symbol).replace(/'/g, "\\'")}')">
                                ${q.shortname || q.longname || q.symbol} (${q.symbol})
                            </div>
                        `).join('') || `<div class="search-item" onclick="selectStock('${query.toUpperCase()}', '${query.toUpperCase()}')">Use "${query.toUpperCase()}" as ticker</div>`;
                    } else {
                        results.innerHTML = `<div class="search-item" onclick="selectStock('${query.toUpperCase()}', '${query.toUpperCase()}')">Use "${query.toUpperCase()}" as ticker</div>`;
                    }
                })
                .catch(() => {
                    const ticker = query.toUpperCase();
                    results.innerHTML = `<div class="search-item" onclick="selectStock('${ticker}', '${ticker}')">Use "${ticker}" as ticker</div>`;
                });
            }, 300);
        }
        function selectStock(ticker, name) {
            document.getElementById('ticker').value = ticker.toUpperCase();
            document.getElementById('stockName').value = name;
            document.getElementById('stock-search').value = '';
            document.getElementById('stock-results').style.display = 'none';
        }
        function searchWeather(query) {
            clearTimeout(searchTimeout);
            if (query.length < 2) {
                document.getElementById('weather-results').style.display = 'none';
                return;
            }
            const results = document.getElementById('weather-results');
            results.innerHTML = '<div class="search-item">Searching...</div>';
            results.style.display = 'block';
            searchTimeout = setTimeout(() => {
                fetch(`https://geocoding-api.open-meteo.com/v1/search?name=${encodeURIComponent(query)}&count=5&language=en&format=json`)
                .then(r => r.json())
                .then(d => {
                    results.innerHTML = (d.results || []).map(city => `
                        <div class="search-item" onclick="selectWeather('${city.name.replace(/'/g, "\\'")}', ${city.latitude}, ${city.longitude})">
                            ${city.name}${city.admin1 ? ', ' + city.admin1 : ''}${city.country ? ' (' + city.country + ')' : ''}
                        </div>
                    `).join('') || '<div class="search-item">No results</div>';
                })
                .catch(() => results.innerHTML = '<div class="search-item">Error searching</div>');
            }, 300);
        }
        function selectWeather(location, lat, lon) {
            document.getElementById('location').value = location;
            document.getElementById('latitude').value = lat;
            document.getElementById('longitude').value = lon;
            document.getElementById('weather-search').value = '';
            document.getElementById('weather-results').style.display = 'none';
        }
        function toggleCustomMode(mode) {
            const manualFields = document.getElementById('manual-fields');
            const countdownFields = document.getElementById('countdown-fields');
            const httpFields = document.getElementById('http-fields');

            manualFields.style.display = mode === 'manual' ? 'block' : 'none';
            countdownFields.style.display = mode === 'countdown' ? 'block' : 'none';
            httpFields.style.display = mode === 'http' ? 'block' : 'none';
        }
        function toggleCountdownType(type) {
            const timeField = document.getElementById('time-field');
            timeField.style.display = type === 'datetime' ? 'block' : 'none';
        }
        function saveModule() {
            const type = currentModule.type;
            const isNew = !modules.find(m => m.id === currentModule.id);
            let data = {id: currentModule.id, type: type};

            // Get refresh interval (applies to all module types except quad)
            if (type !== 'quad') {
                const refreshIntervalInput = document.getElementById('refreshInterval');
                if (refreshIntervalInput) {
                    data.refreshInterval = parseInt(refreshIntervalInput.value);
                }
            }

            if (type === 'crypto') {
                data.cryptoId = document.getElementById('cryptoId').value;
                data.cryptoSymbol = document.getElementById('cryptoSymbol').value;
                data.cryptoName = document.getElementById('cryptoName').value;
                const decimalsInput = document.getElementById('decimals').value;
                if (decimalsInput !== '' && decimalsInput !== 'auto') {
                    data.decimals = parseInt(decimalsInput);
                }
                if (!data.cryptoId) { showMessage('Please search and select a cryptocurrency', 'error'); return; }
            } else if (type === 'stock') {
                data.ticker = document.getElementById('ticker').value;
                data.name = document.getElementById('stockName').value;
                const decimalsInput = document.getElementById('decimals').value;
                if (decimalsInput !== '' && decimalsInput !== 'auto') {
                    data.decimals = parseInt(decimalsInput);
                }
                if (!data.ticker) { showMessage('Please enter a ticker symbol', 'error'); return; }
            } else if (type === 'weather') {
                data.location = document.getElementById('location').value;
                data.latitude = parseFloat(document.getElementById('latitude').value);
                data.longitude = parseFloat(document.getElementById('longitude').value);
                if (!data.location || data.latitude === 0 || data.longitude === 0) {
                    showMessage('Please search and select a location', 'error');
                    return;
                }
            } else if (type === 'custom') {
                data.label = document.getElementById('label').value;
                data.value = parseFloat(document.getElementById('value').value) || 0;
                data.unit = document.getElementById('unit').value;
                data.decimals = parseInt(document.getElementById('decimals').value);

                if (!data.label) { showMessage('Please enter a label', 'error'); return; }
            } else if (type === 'countdown') {
                data.label = document.getElementById('label').value;
                data.targetDate = document.getElementById('targetDate').value;
                data.countdownType = document.getElementById('countdownType').value;
                data.targetTime = (data.countdownType === 'datetime') ? document.getElementById('targetTime').value : '00:00';
                data.decimals = parseInt(document.getElementById('decimals').value);

                if (!data.targetDate) { showMessage('Please select a target date', 'error'); return; }
                if (!data.label) { showMessage('Please enter an event name', 'error'); return; }
            } else if (type === 'http') {
                data.label = document.getElementById('label').value;
                data.url = document.getElementById('url').value;
                data.jsonPath = document.getElementById('jsonPath').value || '';
                data.unit = document.getElementById('unit').value;

                if (!data.url) { showMessage('Please enter an API URL', 'error'); return; }
                if (!data.label) { showMessage('Please enter a label', 'error'); return; }
            } else if (type === 'transit') {
                data.routeName = document.getElementById('routeName').value;
                data.stopName = document.getElementById('stopName').value;
                data.apiUrl = document.getElementById('apiUrl').value;
                data.jsonPath = document.getElementById('jsonPath').value || '';
                data.apiKey = document.getElementById('apiKey').value || '';

                if (!data.routeName) { showMessage('Please enter a route name', 'error'); return; }
                if (!data.apiUrl) { showMessage('Please enter an API URL', 'error'); return; }
            } else if (type === 'quad') {
                data.slot1 = document.getElementById('slot1').value;
                data.slot2 = document.getElementById('slot2').value;
                data.slot3 = document.getElementById('slot3').value;
                data.slot4 = document.getElementById('slot4').value;
            }
            const url = isNew ? '/api/modules' : '/api/modules/update';
            fetch(url, {
                method: 'POST',
                headers: {'Authorization': token, 'Content-Type': 'application/json'},
                body: JSON.stringify(data)
            })
            .then(r => r.json())
            .then(d => {
                if (d.success || d.id) {
                    closeModal();
                    loadModules();
                    showMessage(isNew ? 'Module added successfully' : 'Module updated successfully', 'success');
                } else {
                    showMessage(d.error || 'Failed to save module', 'error');
                }
            })
            .catch(e => showMessage('Failed to save module', 'error'));
        }
        function deleteModule(id) {
            if (!confirm('Are you sure you want to delete this module?')) return;
            fetch('/api/modules/delete', {
                method: 'POST',
                headers: {'Authorization': token, 'Content-Type': 'application/json'},
                body: JSON.stringify({id: id})
            })
            .then(r => r.json())
            .then(d => {
                if (d.success) {
                    loadModules();
                    showMessage('Module deleted', 'success');
                } else {
                    showMessage(d.error || 'Failed to delete module', 'error');
                }
            })
            .catch(e => showMessage('Failed to delete module', 'error'));
        }
        function saveDeviceSettings() {
            const currency = document.getElementById('currency').value;
            const thousandSep = document.getElementById('thousandSep').value;
            fetch('/api/config', {
                method: 'POST',
                headers: {'Authorization': token, 'Content-Type': 'application/json'},
                body: JSON.stringify({device: {currency: currency, thousandSep: thousandSep}})
            })
            .then(r => r.json())
            .then(d => showMessage('Settings saved', 'success'))
            .catch(e => showMessage('Failed to save settings', 'error'));
        }
        function restartDevice() {
            if (!confirm('Restart device? This will disconnect you.')) return;
            fetch('/api/restart', {
                method: 'POST',
                headers: {'Authorization': token}
            })
            .then(() => {
                showMessage('Device restarting...', 'success');
                setTimeout(logout, 2000);
            })
            .catch(e => showMessage('Failed to restart', 'error'));
        }
        function factoryReset() {
            if (!confirm('Factory reset? This will erase ALL settings and data!')) return;
            if (!confirm('Are you ABSOLUTELY sure? This cannot be undone!')) return;
            fetch('/api/factory-reset', {
                method: 'POST',
                headers: {'Authorization': token}
            })
            .then(() => {
                showMessage('Factory reset initiated...', 'success');
                setTimeout(logout, 2000);
            })
            .catch(e => showMessage('Failed to reset', 'error'));
        }
        function closeModal() {
            document.getElementById('module-modal').classList.remove('active');
            currentModule = null;
        }
        function showMessage(msg, type = 'success') {
            const el = document.getElementById('message');
            el.textContent = msg;
            el.className = `message ${type}`;
            el.classList.remove('hidden');
            setTimeout(() => el.classList.add('hidden'), 5000);
        }
    </script>
</body>
</html>)rawliteral";

NetworkManager::NetworkManager()
    : server(nullptr), isAPMode(false), isSettingsMode(false), lastReconnectAttempt(0),
      cachedScanResults("[]"), lastScanTime(0), scanInProgress(false),
      clientWasConnected(false) {
}

NetworkManager::~NetworkManager() {
    if (server) delete server;
}

String NetworkManager::generateAnimalName() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    // Use last byte to select from 64 animals
    uint8_t index = mac[5] % 64;
    return String(ANIMAL_NAMES[index]);
}

String NetworkManager::generateSSID() {
    uint8_t mac[6];
    WiFi.macAddress(mac);

    // Get animal name
    animalName = generateAnimalName();

    // Get 2-char suffix from MAC for uniqueness
    char suffix[3];
    snprintf(suffix, sizeof(suffix), "%02X", mac[4]);

    // Build SSID: DT-PandaA3 (shorter to fit password on screen)
    apName = "DT-" + animalName + String(suffix);
    return apName;
}

String NetworkManager::generatePassword() {
    uint8_t mac[6];
    WiFi.macAddress(mac);

    // Format: DT + last 3 MAC bytes + !
    // Example: DTA3F2E1!
    char macStr[7];
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X",
             mac[3], mac[4], mac[5]);

    apPassword = "DT" + String(macStr) + "!";
    return apPassword;
}

bool NetworkManager::hasClientConnected() {
    return WiFi.softAPgetStationNum() > 0;
}

bool NetworkManager::connectWiFi(const char* ssid, const char* password, uint16_t timeout) {
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeout) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        isAPMode = false;
        return true;
    } else {
        Serial.println("WiFi connection failed");
        return false;
    }
}

void NetworkManager::startConfigAP() {
    // Generate unique SSID and password
    generateSSID();
    generatePassword();

    Serial.println("\n╔════════════════════════════════╗");
    Serial.println("║   CONFIG MODE STARTED          ║");
    Serial.println("╚════════════════════════════════╝");
    Serial.println();
    Serial.print("📡 Network: ");
    Serial.println(apName);
    Serial.print("🔐 Password: ");
    Serial.println(apPassword);
    Serial.print("🦁 Animal: ");
    Serial.println(animalName);
    Serial.println();

    // Start AP with password
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str(), apPassword.c_str());

    IPAddress IP = WiFi.softAPIP();
    Serial.print("🌐 AP IP: ");
    Serial.println(IP);

    delay(1000);

    // Switch to AP+STA for scanning
    WiFi.mode(WIFI_AP_STA);

    // Start mDNS responder
    if (!MDNS.begin("dt")) {
        Serial.println("⚠️  mDNS failed to start");
    } else {
        Serial.println("✓ mDNS started: dt.local");
        MDNS.addService("http", "tcp", 80);
    }

    // Setup web server
    setupWebServer();

    cachedScanResults = "[]";
    scanInProgress = false;
    lastScanTime = millis();
    clientWasConnected = false;

    Serial.println();
    Serial.println("📱 Scan QR code on display");
    Serial.println("   or connect manually");
    Serial.println();
    Serial.println("🌐 Once connected, open:");
    Serial.println("   http://dt.local");
    Serial.println("   http://192.168.4.1");
    Serial.println();

    isAPMode = true;
}

void NetworkManager::setupWebServer() {
    server = new WebServer(80);

    // Root page
    server->on("/", [this]() {
        server->send_P(200, "text/html", PORTAL_HTML);
    });

    // Scan endpoint
    server->on("/scan", [this]() {
        handleScan();
    });

    // Save endpoint
    server->on("/save", HTTP_POST, [this]() {
        handleSave();
    });

    server->begin();
    Serial.println("Web server started");
}

void NetworkManager::handleScan() {
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->send(200, "application/json", cachedScanResults);
}

void NetworkManager::handleSave() {
    if (server->hasArg("plain")) {
        String body = server->arg("plain");

        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, body);

        if (!error) {
            config["wifi"]["ssid"] = doc["ssid"].as<String>();
            config["wifi"]["password"] = doc["password"].as<String>();
            config["device"]["activeModule"] = doc["module"].as<String>();

            saveConfiguration();

            server->send(200, "text/plain", "OK");

            delay(1000);
            ESP.restart();
        } else {
            server->send(400, "text/plain", "Invalid JSON");
        }
    } else {
        server->send(400, "text/plain", "No data");
    }
}

void NetworkManager::stopConfigAP() {
    if (server) {
        delete server;
        server = nullptr;
    }
    WiFi.softAPdisconnect(true);
    isAPMode = false;
}

bool NetworkManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void NetworkManager::reconnect() {
    if (isAPMode) return;

    unsigned long now = millis();
    if (now - lastReconnectAttempt < 30000) return;

    lastReconnectAttempt = now;

    String ssid = config["wifi"]["ssid"] | "";
    String password = config["wifi"]["password"] | "";

    if (ssid.length() == 0) return;

    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.begin(ssid.c_str(), password.c_str());
}

void NetworkManager::handleClient() {
    if (server) {
        server->handleClient();
    }

    if (isAPMode) {
        updateScanResults();
    }
}

bool NetworkManager::httpGet(const char* url, String& response, String& errorMsg) {
    HTTPClient http;
    WiFiClientSecure* secureClient = nullptr;

    // Detect if URL is HTTPS or HTTP
    String urlStr = String(url);
    bool isHttps = urlStr.startsWith("https://");

    if (isHttps) {
        secureClient = new WiFiClientSecure;
        if (!secureClient) {
            errorMsg = "Out of memory";
            return false;
        }
        secureClient->setInsecure();
        http.begin(*secureClient, url);
    } else {
        // Plain HTTP - no need for secure client
        http.begin(url);
    }

    http.setTimeout(15000);
    int httpCode = http.GET();

    bool success = false;
    if (httpCode == HTTP_CODE_OK) {
        response = http.getString();
        success = true;
    } else {
        errorMsg = "HTTP " + String(httpCode);
    }

    http.end();
    if (secureClient) delete secureClient;

    return success;
}

bool NetworkManager::httpGetWithHeaders(const char* url, String& response, String& errorMsg) {
    WiFiClientSecure* client = new WiFiClientSecure;
    if (!client) {
        errorMsg = "Out of memory";
        return false;
    }

    client->setInsecure();

    HTTPClient https;
    https.begin(*client, url);
    https.setTimeout(15000);

    // Add User-Agent header for Yahoo Finance API
    https.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");
    https.addHeader("Accept", "application/json");
    https.addHeader("Accept-Encoding", "gzip, deflate");
    https.addHeader("DNT", "1");
    https.addHeader("Connection", "keep-alive");
    https.addHeader("Upgrade-Insecure-Requests", "1");

    int httpCode = https.GET();

    if (httpCode == HTTP_CODE_OK) {
        response = https.getString();
        https.end();
        delete client;
        return true;
    } else {
        errorMsg = "HTTP " + String(httpCode);
        https.end();
        delete client;
        return false;
    }
}

void NetworkManager::startWiFiScan() {
    Serial.println("Starting WiFi scan...");

    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        delay(500);
    }

    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    // Increased to 500ms per channel for more reliable scanning
    int result = WiFi.scanNetworks(true, false, false, 500);

    if (result == WIFI_SCAN_RUNNING) {
        scanInProgress = true;
        lastScanTime = millis();
        Serial.println("Scan started");
    } else {
        Serial.println("Scan failed to start");
    }
}

void NetworkManager::updateScanResults() {
    if (!scanInProgress) {
        unsigned long timeSinceLastScan = millis() - lastScanTime;
        // Wait 10 seconds after boot for WiFi to stabilize, then rescan every 30 seconds
        if ((lastScanTime < 10000 && millis() > 10000) || timeSinceLastScan > 30000) {
            startWiFiScan();
        }
        return;
    }

    int n = WiFi.scanComplete();

    if (n == WIFI_SCAN_RUNNING) {
        unsigned long scanDuration = millis() - lastScanTime;
        if (scanDuration > 15000) {
            Serial.println("Scan timeout");
            WiFi.scanDelete();
            cachedScanResults = "[]";
            scanInProgress = false;
        }
        return;
    }

    if (n == WIFI_SCAN_FAILED) {
        Serial.println("Scan failed");
        cachedScanResults = "[]";
        scanInProgress = false;
        WiFi.scanDelete();
        return;
    }

    if (n >= 0) {
        Serial.print("Found ");
        Serial.print(n);
        Serial.println(" networks");

        String json = "[";
        for (int i = 0; i < n && i < 20; i++) {
            if (i > 0) json += ",";
            String ssid = WiFi.SSID(i);
            ssid.replace("\"", "\\\"");
            json += "{\"ssid\":\"" + ssid + "\",";
            json += "\"rssi\":" + String(WiFi.RSSI(i)) + "}";
        }
        json += "]";

        cachedScanResults = json;
        scanInProgress = false;
        lastScanTime = millis();

        Serial.println("Scan complete");
        WiFi.scanDelete();
    }
}

// ============================================
// Settings Server (for normal operation mode)
// ============================================

void NetworkManager::startSettingsServer() {
    if (server || isAPMode) {
        Serial.println("Cannot start settings server: server already running or in AP mode");
        return;
    }

    Serial.println("\n=== Starting Settings Server ===");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    isSettingsMode = true;
    setupSettingsServer();

    Serial.println("Settings server started");
    Serial.println("================================\n");
}

void NetworkManager::stopSettingsServer() {
    if (server && isSettingsMode) {
        delete server;
        server = nullptr;
        isSettingsMode = false;
        Serial.println("Settings server stopped");
    }
}

bool NetworkManager::isSettingsServerRunning() {
    return isSettingsMode && (server != nullptr);
}

String NetworkManager::getLocalIP() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

void NetworkManager::setupSettingsServer() {
    Serial.println("\n=== Setting up Settings Server ===");
    server = new WebServer(80);
    Serial.println("WebServer created on port 80");

    // Settings page root - serve new dynamic interface
    server->on("/", [this]() {
        // For now, redirect to old interface until we replace SETTINGS_HTML
        // In production, we'd serve the new HTML here
        handleSettingsRoot();
    });
    Serial.println("Registered: /");

    // API endpoints
    server->on("/api/validate", HTTP_POST, [this]() {
        handleValidateCode();
    });

    server->on("/api/config", HTTP_GET, [this]() {
        handleGetConfig();
    });

    server->on("/api/config", HTTP_POST, [this]() {
        handleUpdateConfig();
    });

    server->on("/api/restart", HTTP_POST, [this]() {
        handleRestart();
    });

    server->on("/api/factory-reset", HTTP_POST, [this]() {
        handleFactoryReset();
    });

    // Stock search proxy (no auth required) - bypasses CORS
    server->on("/api/stock-search", HTTP_GET, [this]() {
        handleStockSearch();
    });

    // Version endpoint (no auth required) - check firmware version
    auto versionHandler = [this]() {
        String response = "{";
        response += "\"version\":\"" + String(FIRMWARE_VERSION) + "\",";
        response += "\"build\":\"" + String(BUILD_NAME) + " - " + String(BUILD_DATE) + "\",";
        response += "\"uptime\":" + String(millis() / 1000);
        response += "}";
        server->send(200, "application/json", response);
    };

    server->on("/api/version/", HTTP_GET, versionHandler);
    server->on("/api/version", HTTP_GET, versionHandler);
    Serial.println("Registered: /api/version and /api/version/");

    // Debug endpoint - trigger stock fetch and show diagnostics
    // Register both with and without trailing slash
    auto testStockHandler = [this]() {
        // Check authorization
        String token = server->header("Authorization");
        if (!security.validateSession(token)) {
            server->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }

        extern Scheduler scheduler;

        Serial.println("\n=== MANUAL STOCK FETCH TEST ===");
        String ticker = config["modules"]["stock"]["ticker"] | "AAPL";
        Serial.print("Ticker: ");
        Serial.println(ticker);

        // Force a fetch
        scheduler.requestFetch("stock", true);

        // Wait a bit for fetch to complete
        delay(3000);

        // Get status
        JsonObject stock = config["modules"]["stock"];
        float price = stock["value"] | 0.0;
        float change = stock["change"] | 0.0;
        unsigned long lastUpdate = stock["lastUpdate"] | 0;
        bool lastSuccess = stock["lastSuccess"] | false;

        String response = "{";
        response += "\"ticker\":\"" + String(ticker) + "\",";
        response += "\"price\":" + String(price, 2) + ",";
        response += "\"change\":" + String(change, 2) + ",";
        response += "\"lastUpdate\":" + String(lastUpdate) + ",";
        response += "\"lastSuccess\":" + String(lastSuccess ? "true" : "false");
        response += "}";

        Serial.println("Test response:");
        Serial.println(response);
        Serial.println("=== END TEST ===\n");

        server->send(200, "application/json", response);
    };

    server->on("/api/test-stock", HTTP_POST, testStockHandler);
    server->on("/api/test-stock/", HTTP_POST, testStockHandler);

    // Force stock fetch endpoint (no auth for debugging)
    server->on("/api/force-stock", HTTP_GET, [this]() {
        extern Scheduler scheduler;

        Serial.println("\n=== /api/force-stock called ===");

        // Check if stock module is registered
        bool hasStock = scheduler.hasModule("stock");
        bool hasBitcoin = scheduler.hasModule("bitcoin");
        int moduleCount = scheduler.getModuleCount();

        // Force a stock fetch
        scheduler.requestFetch("stock", true);

        // Wait a moment
        delay(3000);

        // Check result
        JsonObject stock = config["modules"]["stock"];
        unsigned long lastUpdate = stock["lastUpdate"] | 0;
        float value = stock["value"] | 0.0;
        bool success = stock["lastSuccess"] | false;
        String error = stock["lastError"] | "";

        String response = "{";
        response += "\"triggered\":true,";
        response += "\"moduleCount\":" + String(moduleCount) + ",";
        response += "\"hasStock\":" + String(hasStock ? "true" : "false") + ",";
        response += "\"hasBitcoin\":" + String(hasBitcoin ? "true" : "false") + ",";
        response += "\"lastUpdate\":" + String(lastUpdate) + ",";
        response += "\"value\":" + String(value) + ",";
        response += "\"lastSuccess\":" + String(success ? "true" : "false") + ",";
        response += "\"lastError\":\"" + error + "\"";
        response += "}";

        Serial.println("=== Force stock result ===");
        Serial.println(response);

        server->send(200, "application/json", response);
    });

    // Force weather fetch endpoint (no auth for debugging)
    server->on("/api/force-weather", HTTP_GET, [this]() {
        extern Scheduler scheduler;

        Serial.println("\n=== /api/force-weather called ===");

        // Find weather module ID dynamically
        String weatherModuleId = "";
        JsonArray moduleOrder = config["device"]["moduleOrder"];
        for (JsonVariant v : moduleOrder) {
            String id = v.as<String>();
            if (id.startsWith("weather")) {
                weatherModuleId = id;
                break;
            }
        }

        // Check if weather module is registered
        bool hasWeather = false;
        if (weatherModuleId.length() > 0) {
            hasWeather = scheduler.hasModule(weatherModuleId.c_str());
        }
        int moduleCount = scheduler.getModuleCount();

        // Force a weather fetch if found
        if (weatherModuleId.length() > 0) {
            Serial.print("Found weather module: ");
            Serial.println(weatherModuleId);
            scheduler.requestFetch(weatherModuleId.c_str(), true);
            delay(3000);
        } else {
            Serial.println("No weather module found in moduleOrder");
        }

        // Check result
        JsonObject weather;
        if (weatherModuleId.length() > 0) {
            weather = config["modules"][weatherModuleId];
        }
        unsigned long lastUpdate = weather["lastUpdate"] | 0;
        float temp = weather["temperature"] | 0.0;
        String condition = weather["condition"] | "Unknown";
        String location = weather["location"] | "Unknown";
        float latitude = weather["latitude"] | 0.0;
        float longitude = weather["longitude"] | 0.0;
        bool success = weather["lastSuccess"] | false;
        String error = weather["lastError"] | "";

        String response = "{";
        response += "\"triggered\":true,";
        response += "\"weatherModuleId\":\"" + weatherModuleId + "\",";
        response += "\"moduleCount\":" + String(moduleCount) + ",";
        response += "\"hasWeather\":" + String(hasWeather ? "true" : "false") + ",";
        response += "\"location\":\"" + location + "\",";
        response += "\"latitude\":" + String(latitude, 4) + ",";
        response += "\"longitude\":" + String(longitude, 4) + ",";
        response += "\"lastUpdate\":" + String(lastUpdate) + ",";
        response += "\"temperature\":" + String(temp, 1) + ",";
        response += "\"condition\":\"" + condition + "\",";
        response += "\"lastSuccess\":" + String(success ? "true" : "false") + ",";
        response += "\"lastError\":\"" + error + "\"";
        response += "}";

        Serial.println("=== Force weather result ===");
        Serial.println(response);

        server->send(200, "application/json", response);
    });

    // Debug endpoint to show last save processing
    server->on("/api/debug-save", HTTP_GET, [this]() {
        server->send(200, "text/plain", lastSaveResult);
    });

    // Module list debug (no auth)
    server->on("/modules-debug", HTTP_GET, [this]() {
        extern Scheduler scheduler;
        String html = "<html><head><title>Modules Debug</title>";
        html += "<style>body{font-family:monospace;padding:20px}pre{background:#f5f5f5;padding:10px}</style>";
        html += "</head><body><h1>Modules Debug</h1>";

        // Module order from config
        html += "<h2>Module Order (from config):</h2><pre>";
        JsonArray moduleOrder = config["device"]["moduleOrder"];
        html += "Count: " + String(moduleOrder.size()) + "\n";
        for (JsonVariant v : moduleOrder) {
            html += "- " + v.as<String>() + "\n";
        }
        html += "</pre>";

        // Registered modules from scheduler
        html += "<h2>Registered Modules (in scheduler):</h2><pre>";
        html += "Count: " + String(scheduler.getModuleCount()) + "\n";

        // Find actual weather module ID (could be "weather" or "weather_timestamp")
        String weatherModuleId = "";
        for (JsonVariant v : moduleOrder) {
            String id = v.as<String>();
            if (id.startsWith("weather")) {
                weatherModuleId = id;
                break;
            }
        }

        if (weatherModuleId.length() > 0) {
            html += "Weather module ID: " + weatherModuleId + "\n";
            html += "Has weather registered: " + String(scheduler.hasModule(weatherModuleId.c_str()) ? "YES" : "NO") + "\n";
        } else {
            html += "No weather module found in moduleOrder\n";
        }
        html += "</pre>";

        // Weather module config (using actual ID)
        html += "<h2>Weather Config:</h2><pre>";
        if (weatherModuleId.length() > 0) {
            JsonObject weather = config["modules"][weatherModuleId];
            if (weather.isNull()) {
                html += "Weather config is NULL (ID: " + weatherModuleId + ")\n";
            } else {
                html += "Module ID: " + weatherModuleId + "\n";
                html += "Type: " + String(weather["type"] | "missing") + "\n";
                html += "Location: " + String(weather["location"] | "missing") + "\n";
                html += "Temperature: " + String(weather["temperature"] | 0.0, 1) + "\n";
                html += "Latitude: " + String(weather["latitude"] | 0.0, 4) + "\n";
                html += "Longitude: " + String(weather["longitude"] | 0.0, 4) + "\n";
                html += "Last Update: " + String(weather["lastUpdate"] | 0) + "\n";
            }
        } else {
            html += "No weather module ID found\n";
        }
        html += "</pre>";

        html += "</body></html>";
        server->send(200, "text/html", html);
    });

    // Weather debug page (no auth for debugging)
    server->on("/weather-debug", HTTP_GET, [this]() {
        String html = "<!DOCTYPE html><html><head><title>Weather Debug</title>";
        html += "<style>body{font-family:monospace;padding:20px;max-width:800px;margin:0 auto}";
        html += "button{padding:10px 20px;font-size:16px;margin:10px 0}";
        html += "pre{background:#f5f5f5;padding:15px;border-radius:5px;overflow-x:auto}";
        html += ".result{margin-top:20px}</style></head><body>";
        html += "<h1>Weather Module Debug</h1>";
        html += "<p>Click the button to fetch weather and see the raw data:</p>";
        html += "<button onclick='fetchWeather()'>Fetch Weather Now</button>";
        html += "<div class='result' id='result'></div>";
        html += "<script>function fetchWeather(){";
        html += "document.getElementById('result').innerHTML='<p>Fetching...</p>';";
        html += "fetch('/api/force-weather').then(r=>r.json()).then(d=>{";
        html += "document.getElementById('result').innerHTML='<h2>Result:</h2><pre>'+JSON.stringify(d,null,2)+'</pre>'";
        html += "+'<h3>Summary:</h3><p><strong>Location:</strong> '+d.location+'</p>'";
        html += "+'<p><strong>Temperature:</strong> '+d.temperature+' C</p>'";
        html += "+'<p><strong>Condition:</strong> '+d.condition+'</p>'";
        html += "+'<p><strong>Last Update:</strong> '+d.lastUpdate+' seconds</p>'";
        html += "+'<p><strong>Success:</strong> '+d.lastSuccess+'</p>'";
        html += "+(d.lastError?'<p><strong>Error:</strong> '+d.lastError+'</p>':'');";
        html += "}).catch(e=>{document.getElementById('result').innerHTML='<p style=\"color:red\">Error: '+e+'</p>';});";
        html += "}</script></body></html>";
        server->send(200, "text/html", html);
    });

    // Debug endpoint for weather config (no auth for debugging)
    server->on("/api/weather-config", HTTP_GET, [this]() {
        JsonObject weather = config["modules"]["weather"];

        String response = "Weather config in memory:\n\n";
        response += "location: " + String(weather["location"] | "NOT SET") + "\n";
        response += "latitude: " + String(weather["latitude"] | 0.0, 6) + "\n";
        response += "longitude: " + String(weather["longitude"] | 0.0, 6) + "\n";
        response += "temperature: " + String(weather["temperature"] | 0.0, 1) + "\n";
        response += "condition: " + String(weather["condition"] | "Unknown") + "\n";
        response += "lastUpdate: " + String(weather["lastUpdate"] | 0) + "\n";
        response += "lastSuccess: " + String(weather["lastSuccess"] | false ? "true" : "false") + "\n";

        // Also check what fields actually exist
        response += "\nField existence check:\n";
        response += "  location exists: " + String(weather.containsKey("location") ? "yes" : "no") + "\n";
        response += "  latitude exists: " + String(weather.containsKey("latitude") ? "yes" : "no") + "\n";
        response += "  longitude exists: " + String(weather.containsKey("longitude") ? "yes" : "no") + "\n";

        server->send(200, "text/plain", response);
    });

    // Debug endpoint to show last POST body (no auth for debugging)
    auto debugLastPostHandler = [this]() {
        // Parse the body to check for overflow
        StaticJsonDocument<2048> testDoc;
        DeserializationError err = deserializeJson(testDoc, lastPostBody);

        String response = "Last POST body (length=" + String(lastPostBody.length()) + "):\n\n";
        response += lastPostBody;
        response += "\n\n--- Parse Test ---\n";
        if (err) {
            response += "Parse ERROR: " + String(err.c_str()) + "\n";
        } else {
            response += "Parse OK\n";
            response += "Memory: " + String(testDoc.memoryUsage()) + " / " + String(testDoc.capacity()) + " bytes\n";
            response += "Overflowed: " + String(testDoc.overflowed() ? "YES - DATA LOST!" : "No") + "\n";

            // Check if weather fields exist
            if (testDoc.containsKey("modules") && testDoc["modules"].containsKey("weather")) {
                JsonObject weather = testDoc["modules"]["weather"];
                response += "\nWeather in parsed doc:\n";
                response += "  location exists: " + String(weather.containsKey("location") ? "yes" : "no") + "\n";
                response += "  latitude exists: " + String(weather.containsKey("latitude") ? "yes" : "no") + "\n";
                response += "  longitude exists: " + String(weather.containsKey("longitude") ? "yes" : "no") + "\n";
                if (weather.containsKey("latitude")) {
                    response += "  latitude value: " + String(weather["latitude"].as<float>(), 6) + "\n";
                }
                if (weather.containsKey("longitude")) {
                    response += "  longitude value: " + String(weather["longitude"].as<float>(), 6) + "\n";
                }
            }
        }

        server->send(200, "text/plain", response);
    };
    server->on("/api/debug-last-post", HTTP_GET, debugLastPostHandler);
    server->on("/api/debug-last-post/", HTTP_GET, debugLastPostHandler);

    // Status endpoint - check if scheduler is working
    server->on("/api/status", HTTP_GET, [this]() {
        extern Scheduler scheduler;

        unsigned long now = millis() / 1000;

        JsonObject stock = config["modules"]["stock"];
        JsonObject bitcoin = config["modules"]["bitcoin"];
        unsigned long stockLastUpdate = stock["lastUpdate"] | 0;
        unsigned long btcLastUpdate = bitcoin["lastUpdate"] | 0;

        // Check config
        bool hasStockConfig = stock["ticker"].as<String>().length() > 0;
        bool hasBitcoinConfig = bitcoin["cryptoId"].as<String>().length() > 0;

        String response = "{";
        response += "\"stock_lastUpdate\":" + String(stockLastUpdate) + ",";
        response += "\"stock_value\":" + String(stock["value"] | 0.0) + ",";
        response += "\"stock_lastSuccess\":" + String(stock["lastSuccess"] | false ? "true" : "false") + ",";
        response += "\"stock_ticker\":\"" + stock["ticker"].as<String>() + "\",";
        response += "\"stock_config_exists\":" + String(hasStockConfig ? "true" : "false") + ",";
        response += "\"bitcoin_lastUpdate\":" + String(btcLastUpdate) + ",";
        response += "\"bitcoin_value\":" + String(bitcoin["value"] | 0.0) + ",";
        response += "\"bitcoin_lastSuccess\":" + String(bitcoin["lastSuccess"] | false ? "true" : "false") + ",";
        response += "\"bitcoin_config_exists\":" + String(hasBitcoinConfig ? "true" : "false") + ",";
        response += "\"current_time\":" + String(now) + ",";
        response += "\"stock_time_since_update\":" + String(now - stockLastUpdate) + ",";
        response += "\"bitcoin_time_since_update\":" + String(now - btcLastUpdate) + "";
        response += "}";
        server->send(200, "application/json", response);
    });

    // Debug endpoint (no auth required) - shows crypto module config only
    server->on("/debug", [this]() {
        String html = "<!DOCTYPE html><html><head><title>Debug Config</title>";
        html += "<style>body{font-family:monospace;padding:20px;background:#1e1e1e;color:#d4d4d4}";
        html += "table{border-collapse:collapse;margin:20px 0}";
        html += "td,th{border:1px solid #666;padding:8px 12px;text-align:left}";
        html += "th{background:#2d2d2d}</style></head><body>";
        html += "<h2>Crypto Module Configuration</h2>";
        html += "<p style='color:#888'>v2.6.4 - Auto-Fetch Fix | Auto-refreshes every 3 seconds</p>";
        html += "<table><tr><th>Module</th><th>Field</th><th>Value</th></tr>";

        // Bitcoin module
        JsonObject bitcoin = config["modules"]["bitcoin"];
        html += "<tr><td rowspan='6'>Crypto 1 (bitcoin)</td>";
        html += "<td>cryptoId</td><td>" + String(bitcoin["cryptoId"] | "NOT SET") + "</td></tr>";
        html += "<tr><td>cryptoSymbol</td><td>" + String(bitcoin["cryptoSymbol"] | "NOT SET") + "</td></tr>";
        html += "<tr><td>cryptoName</td><td>" + String(bitcoin["cryptoName"] | "NOT SET") + "</td></tr>";
        html += "<tr><td>value</td><td>$" + String(bitcoin["value"] | 0.0, 2) + "</td></tr>";
        html += "<tr><td>lastUpdate</td><td>" + String(bitcoin["lastUpdate"] | 0) + "</td></tr>";
        html += "<tr><td>lastSuccess</td><td>" + String(bitcoin["lastSuccess"] | false ? "true" : "false") + "</td></tr>";

        // Ethereum module
        JsonObject ethereum = config["modules"]["ethereum"];
        html += "<tr><td rowspan='6'>Crypto 2 (ethereum)</td>";
        html += "<td>cryptoId</td><td>" + String(ethereum["cryptoId"] | "NOT SET") + "</td></tr>";
        html += "<tr><td>cryptoSymbol</td><td>" + String(ethereum["cryptoSymbol"] | "NOT SET") + "</td></tr>";
        html += "<tr><td>cryptoName</td><td>" + String(ethereum["cryptoName"] | "NOT SET") + "</td></tr>";
        html += "<tr><td>value</td><td>$" + String(ethereum["value"] | 0.0, 2) + "</td></tr>";
        html += "<tr><td>lastUpdate</td><td>" + String(ethereum["lastUpdate"] | 0) + "</td></tr>";
        html += "<tr><td>lastSuccess</td><td>" + String(ethereum["lastSuccess"] | false ? "true" : "false") + "</td></tr>";

        html += "</table>";

        html += "<h3>Raw JSON Dump</h3>";
        html += "<pre style='background:#2d2d2d;padding:10px;overflow:auto;max-height:400px'>";
        String configJson;
        serializeJsonPretty(config, configJson);
        configJson.replace("<", "&lt;");
        configJson.replace(">", "&gt;");
        html += configJson;
        html += "</pre>";

        html += "<p>Config memory: " + String(config.memoryUsage()) + " / " + String(config.capacity()) + " bytes</p>";
        if (config.overflowed()) {
            html += "<p style='color:#f44336;font-weight:bold'>⚠ WARNING: Config overflowed!</p>";
        }
        html += "<script>setTimeout(function(){location.reload()},3000);</script>";
        html += "</body></html>";
        server->send(200, "text/html", html);
    });

    // Module Management API endpoints
    // GET /api/modules - List all configured modules
    server->on("/api/modules", HTTP_GET, [this]() {
        String token = server->header("Authorization");
        if (!security.validateSession(token)) {
            server->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }

        StaticJsonDocument<6144> response;  // Increased from 4096 to 6144 for more modules
        JsonArray modulesArray = response.createNestedArray("modules");

        // Get module order
        JsonArray moduleOrder = config["device"]["moduleOrder"];

        for (JsonVariant v : moduleOrder) {
            String moduleId = v.as<String>();
            JsonObject moduleConfig = config["modules"][moduleId];

            if (!moduleConfig.isNull()) {
                JsonObject moduleData = modulesArray.createNestedObject();
                moduleData["id"] = moduleId;
                moduleData["type"] = moduleConfig["type"] | "unknown";

                // Copy module-specific fields based on type
                String type = moduleConfig["type"] | "";
                if (type == "crypto") {
                    moduleData["cryptoId"] = moduleConfig["cryptoId"];
                    moduleData["cryptoSymbol"] = moduleConfig["cryptoSymbol"];
                    moduleData["cryptoName"] = moduleConfig["cryptoName"];
                    moduleData["value"] = moduleConfig["value"];
                    moduleData["change24h"] = moduleConfig["change24h"];
                } else if (type == "stock") {
                    moduleData["ticker"] = moduleConfig["ticker"];
                    moduleData["name"] = moduleConfig["name"];
                    moduleData["value"] = moduleConfig["value"];
                    moduleData["change"] = moduleConfig["change"];
                } else if (type == "weather") {
                    moduleData["location"] = moduleConfig["location"];
                    moduleData["latitude"] = moduleConfig["latitude"];
                    moduleData["longitude"] = moduleConfig["longitude"];
                    moduleData["temperature"] = moduleConfig["temperature"];
                    moduleData["condition"] = moduleConfig["condition"];
                } else if (type == "custom") {
                    moduleData["label"] = moduleConfig["label"];
                    moduleData["value"] = moduleConfig["value"];
                    moduleData["unit"] = moduleConfig["unit"];
                    moduleData["decimals"] = moduleConfig["decimals"] | -1;
                } else if (type == "countdown") {
                    moduleData["label"] = moduleConfig["label"] | "Event";
                    moduleData["targetDate"] = moduleConfig["targetDate"] | "";
                    moduleData["targetTime"] = moduleConfig["targetTime"] | "00:00";
                    moduleData["countdownType"] = moduleConfig["countdownType"] | "date";
                    moduleData["value"] = moduleConfig["value"] | 0.0;
                    moduleData["decimals"] = moduleConfig["decimals"] | -1;
                } else if (type == "http") {
                    moduleData["label"] = moduleConfig["label"] | "API";
                    moduleData["url"] = moduleConfig["url"] | "";
                    moduleData["jsonPath"] = moduleConfig["jsonPath"] | "";
                    moduleData["unit"] = moduleConfig["unit"] | "";
                    moduleData["value"] = moduleConfig["value"] | 0.0;
                } else if (type == "transit") {
                    moduleData["routeName"] = moduleConfig["routeName"];
                    moduleData["stopName"] = moduleConfig["stopName"];
                    moduleData["apiUrl"] = moduleConfig["apiUrl"];
                    moduleData["jsonPath"] = moduleConfig["jsonPath"];
                    moduleData["apiKey"] = moduleConfig["apiKey"];
                    moduleData["nextArrival"] = moduleConfig["nextArrival"];
                } else if (type == "quad") {
                    moduleData["slot1"] = moduleConfig["slot1"];
                    moduleData["slot2"] = moduleConfig["slot2"];
                    moduleData["slot3"] = moduleConfig["slot3"];
                    moduleData["slot4"] = moduleConfig["slot4"];
                }

                // Common fields for all modules
                moduleData["lastUpdate"] = moduleConfig["lastUpdate"];
                moduleData["lastSuccess"] = moduleConfig["lastSuccess"];
                moduleData["refreshInterval"] = moduleConfig["refreshInterval"];
            }
        }

        // Check for overflow
        if (response.overflowed()) {
            Serial.println("ERROR: /api/modules response overflowed!");
            server->send(500, "application/json", "{\"error\":\"Too many modules\"}");
            return;
        }

        String output;
        size_t len = serializeJson(response, output);

        // Debug: Log response length and check for control characters
        Serial.print("GET /api/modules - Response length: ");
        Serial.print(len);
        Serial.println(" bytes");

        // Check for invalid characters
        for (size_t i = 0; i < output.length(); i++) {
            char c = output.charAt(i);
            if (c < 32 && c != '\n' && c != '\r' && c != '\t') {
                Serial.print("WARNING: Invalid control character at position ");
                Serial.print(i);
                Serial.print(": 0x");
                Serial.println((int)c, HEX);
            }
        }

        server->send(200, "application/json", output);
    });

    // POST /api/modules - Add a new module
    server->on("/api/modules", HTTP_POST, [this]() {
        String token = server->header("Authorization");
        if (!security.validateSession(token)) {
            server->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }

        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, server->arg("plain"));

        if (error) {
            server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        String moduleId = doc["id"] | "";
        String moduleType = doc["type"] | "";

        if (moduleId.length() == 0 || moduleType.length() == 0) {
            server->send(400, "application/json", "{\"error\":\"Missing id or type\"}");
            return;
        }

        // Check if module already exists
        if (config["modules"][moduleId].as<JsonObject>().size() > 0) {
            server->send(409, "application/json", "{\"error\":\"Module already exists\"}");
            return;
        }

        // Create new module in config
        JsonObject newModule = config["modules"][moduleId].to<JsonObject>();
        newModule["type"] = moduleType;

        // Helper to remove control characters but keep URL-safe special chars
        auto cleanControlChars = [](const char* input) -> String {
            String result = "";
            const char* p = input;

            while (*p) {
                unsigned char c = *p;

                // Allow printable ASCII (32-126) which includes URL chars: : / ? = & . [ ]
                // Skip control characters (0-31) and DEL (127)
                if (c >= 32 && c <= 126) {
                    result += (char)c;
                }
                // Also allow high-bit characters for UTF-8 (128-255)
                else if (c >= 128) {
                    result += (char)c;
                }
                // Skip control characters
                p++;
            }

            return result;
        };

        // Helper to sanitize strings (remove control characters and convert UTF-8 to ASCII)
        auto sanitize = [](const char* input) -> String {
            String result = "";
            const char* p = input;

            while (*p) {
                unsigned char c = *p;

                // Single-byte ASCII character
                if ((c & 0x80) == 0) {
                    // Allow printable ASCII chars (space and above, excluding DEL)
                    if (c >= 32 && c < 127) {
                        result += (char)c;
                    }
                    p++;
                }
                // 2-byte UTF-8 sequence
                else if ((c & 0xE0) == 0xC0 && *(p+1)) {
                    unsigned char c1 = *p++;
                    unsigned char c2 = *p++;

                    // Czech characters: Ř/ř, Č/č, etc.
                    if (c1 == 0xC5 && c2 == 0x98) result += 'R';  // Ř (uppercase)
                    else if (c1 == 0xC5 && c2 == 0x99) result += 'r';  // ř (lowercase)
                    else if (c1 == 0xC4 && c2 == 0x8C) result += 'C';  // Č (uppercase)
                    else if (c1 == 0xC4 && c2 == 0x8D) result += 'c';  // č (lowercase)
                    else if (c1 == 0xC3 && c2 == 0xA1) result += 'a';  // á
                    else if (c1 == 0xC3 && c2 == 0xA9) result += 'e';  // é
                    else if (c1 == 0xC3 && c2 == 0xAD) result += 'i';  // í
                    else if (c1 == 0xC3 && c2 == 0xB3) result += 'o';  // ó
                    else if (c1 == 0xC3 && c2 == 0xBA) result += 'u';  // ú
                    else if (c1 == 0xC3 && c2 == 0xBD) result += 'y';  // ý
                    // Skip unknown accents
                }
                // Skip 3-byte and 4-byte UTF-8 sequences
                else if ((c & 0xF0) == 0xE0 && *(p+1) && *(p+2)) p += 3;
                else if ((c & 0xF8) == 0xF0 && *(p+1) && *(p+2) && *(p+3)) p += 4;
                else p++;  // Skip invalid sequences
            }

            return result;
        };

        // Set default values based on type
        if (moduleType == "crypto") {
            newModule["cryptoId"] = sanitize(doc["cryptoId"] | "bitcoin");
            newModule["cryptoSymbol"] = sanitize(doc["cryptoSymbol"] | "BTC");
            newModule["cryptoName"] = sanitize(doc["cryptoName"] | "Bitcoin");
            newModule["value"] = 0.0;
            newModule["change24h"] = 0.0;
        } else if (moduleType == "stock") {
            newModule["ticker"] = sanitize(doc["ticker"] | "AAPL");
            newModule["name"] = sanitize(doc["name"] | "Apple Inc.");
            newModule["value"] = 0.0;
            newModule["change"] = 0.0;
        } else if (moduleType == "weather") {
            newModule["location"] = sanitize(doc["location"] | "San Francisco");
            newModule["latitude"] = doc["latitude"] | 0.0;
            newModule["longitude"] = doc["longitude"] | 0.0;
            newModule["temperature"] = 0.0;
            newModule["condition"] = "Unknown";
        } else if (moduleType == "custom") {
            newModule["label"] = sanitize(doc["label"] | "My Metric");
            newModule["value"] = doc["value"] | 0.0;
            newModule["unit"] = sanitize(doc["unit"] | "units");
            newModule["decimals"] = doc["decimals"] | -1;
        } else if (moduleType == "countdown") {
            newModule["label"] = sanitize(doc["label"] | "Event");
            String targetDate = doc["targetDate"] | "";
            String targetTime = doc["targetTime"] | "00:00";
            String countdownType = doc["countdownType"] | "date";
            newModule["targetDate"] = targetDate;
            newModule["targetTime"] = targetTime;
            newModule["countdownType"] = countdownType;
            newModule["value"] = 0.0;
            newModule["decimals"] = doc["decimals"] | -1;
        } else if (moduleType == "http") {
            newModule["label"] = sanitize(doc["label"] | "API Data");
            newModule["url"] = cleanControlChars(doc["url"] | "");
            newModule["jsonPath"] = cleanControlChars(doc["jsonPath"] | "");
            newModule["unit"] = sanitize(doc["unit"] | "");
            newModule["value"] = 0.0;
        } else if (moduleType == "transit") {
            newModule["routeName"] = sanitize(doc["routeName"] | "Transit");
            newModule["stopName"] = sanitize(doc["stopName"] | "");
            newModule["apiUrl"] = cleanControlChars(doc["apiUrl"] | "");
            newModule["jsonPath"] = cleanControlChars(doc["jsonPath"] | "");
            newModule["apiKey"] = cleanControlChars(doc["apiKey"] | "");
            newModule["nextArrival"] = 0.0;
        } else if (moduleType == "quad") {
            newModule["slot1"] = doc["slot1"] | "";
            newModule["slot2"] = doc["slot2"] | "";
            newModule["slot3"] = doc["slot3"] | "";
            newModule["slot4"] = doc["slot4"] | "";
        }

        // Set refresh interval (with validation) - applies to all types
        uint16_t refreshInterval = doc["refreshInterval"] | 300;  // Default 5 min
        if (refreshInterval < 10) refreshInterval = 10;  // Min 10s
        if (refreshInterval > 3600) refreshInterval = 3600;  // Max 1 hour
        newModule["refreshInterval"] = refreshInterval;

        newModule["lastUpdate"] = 0;
        newModule["lastSuccess"] = false;

        // Add to module order
        JsonArray moduleOrder = config["device"]["moduleOrder"];
        moduleOrder.add(moduleId);

        // Save configuration
        saveConfiguration(true);

        // Reload modules in scheduler
        extern Scheduler scheduler;
        scheduler.loadModulesFromConfig();

        // Trigger immediate fetch for the new module
        scheduler.requestFetch(moduleId.c_str(), true);

        server->send(201, "application/json", "{\"success\":true,\"id\":\"" + moduleId + "\"}");
    });

    // DELETE /api/modules/{id} - Remove a module
    server->on("/api/modules/delete", HTTP_POST, [this]() {
        String token = server->header("Authorization");
        if (!security.validateSession(token)) {
            server->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }

        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, server->arg("plain"));

        if (error) {
            server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        String moduleId = doc["id"] | "";
        if (moduleId.length() == 0) {
            server->send(400, "application/json", "{\"error\":\"Missing id\"}");
            return;
        }

        // Remove from module order
        JsonArray moduleOrder = config["device"]["moduleOrder"];
        for (size_t i = 0; i < moduleOrder.size(); i++) {
            if (moduleOrder[i].as<String>() == moduleId) {
                // Create new array without this element
                JsonArray newOrder = config["device"].createNestedArray("moduleOrder_temp");
                for (size_t j = 0; j < moduleOrder.size(); j++) {
                    if (j != i) {
                        newOrder.add(moduleOrder[j]);
                    }
                }
                config["device"].remove("moduleOrder");
                config["device"]["moduleOrder"] = newOrder;
                config["device"].remove("moduleOrder_temp");
                break;
            }
        }

        // Remove module config
        config["modules"].remove(moduleId);

        // Unregister from scheduler
        extern Scheduler scheduler;
        scheduler.unregisterModule(moduleId.c_str());

        // Save configuration
        saveConfiguration(true);

        server->send(200, "application/json", "{\"success\":true}");
    });

    // PUT /api/modules/order - Update module display order
    server->on("/api/modules/order", HTTP_POST, [this]() {
        String token = server->header("Authorization");
        if (!security.validateSession(token)) {
            server->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }

        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, server->arg("plain"));

        if (error) {
            server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        JsonArray newOrder = doc["order"];
        if (newOrder.isNull() || newOrder.size() == 0) {
            server->send(400, "application/json", "{\"error\":\"Missing or empty order array\"}");
            return;
        }

        // Update module order
        config["device"].remove("moduleOrder");
        JsonArray moduleOrder = config["device"].createNestedArray("moduleOrder");
        for (JsonVariant v : newOrder) {
            moduleOrder.add(v.as<String>());
        }

        // Save configuration
        saveConfiguration(true);

        server->send(200, "application/json", "{\"success\":true}");
    });

    // PUT /api/modules/{id} - Update module configuration
    server->on("/api/modules/update", HTTP_POST, [this]() {
        String token = server->header("Authorization");
        if (!security.validateSession(token)) {
            server->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }

        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, server->arg("plain"));

        if (error) {
            server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        String moduleId = doc["id"] | "";
        if (moduleId.length() == 0) {
            server->send(400, "application/json", "{\"error\":\"Missing id\"}");
            return;
        }

        JsonObject module = config["modules"][moduleId];
        if (module.isNull()) {
            server->send(404, "application/json", "{\"error\":\"Module not found\"}");
            return;
        }

        String moduleType = module["type"] | "";

        // Helper to remove control characters but keep URL-safe special chars
        auto cleanControlChars = [](const char* input) -> String {
            String result = "";
            const char* p = input;

            while (*p) {
                unsigned char c = *p;

                // Allow printable ASCII (32-126) which includes URL chars: : / ? = & . [ ]
                // Skip control characters (0-31) and DEL (127)
                if (c >= 32 && c <= 126) {
                    result += (char)c;
                }
                // Also allow high-bit characters for UTF-8 (128-255)
                else if (c >= 128) {
                    result += (char)c;
                }
                // Skip control characters
                p++;
            }

            return result;
        };

        // Helper to sanitize strings (remove control characters and convert UTF-8 to ASCII)
        auto sanitize = [](const char* input) -> String {
            String result = "";
            const char* p = input;

            while (*p) {
                unsigned char c = *p;

                // Single-byte ASCII character
                if ((c & 0x80) == 0) {
                    // Allow printable ASCII chars (space and above, excluding DEL)
                    if (c >= 32 && c < 127) {
                        result += (char)c;
                    }
                    p++;
                }
                // 2-byte UTF-8 sequence
                else if ((c & 0xE0) == 0xC0 && *(p+1)) {
                    unsigned char c1 = *p++;
                    unsigned char c2 = *p++;

                    // Czech characters: Ř/ř, Č/č, etc.
                    if (c1 == 0xC5 && c2 == 0x98) result += 'R';  // Ř (uppercase)
                    else if (c1 == 0xC5 && c2 == 0x99) result += 'r';  // ř (lowercase)
                    else if (c1 == 0xC4 && c2 == 0x8C) result += 'C';  // Č (uppercase)
                    else if (c1 == 0xC4 && c2 == 0x8D) result += 'c';  // č (lowercase)
                    else if (c1 == 0xC3 && c2 == 0xA1) result += 'a';  // á
                    else if (c1 == 0xC3 && c2 == 0xA9) result += 'e';  // é
                    else if (c1 == 0xC3 && c2 == 0xAD) result += 'i';  // í
                    else if (c1 == 0xC3 && c2 == 0xB3) result += 'o';  // ó
                    else if (c1 == 0xC3 && c2 == 0xBA) result += 'u';  // ú
                    else if (c1 == 0xC3 && c2 == 0xBD) result += 'y';  // ý
                    // Skip unknown accents
                }
                // Skip 3-byte and 4-byte UTF-8 sequences
                else if ((c & 0xF0) == 0xE0 && *(p+1) && *(p+2)) p += 3;
                else if ((c & 0xF8) == 0xF0 && *(p+1) && *(p+2) && *(p+3)) p += 4;
                else p++;  // Skip invalid sequences
            }

            return result;
        };

        // Update fields based on type
        if (moduleType == "crypto") {
            if (doc.containsKey("cryptoId")) module["cryptoId"] = sanitize(doc["cryptoId"]);
            if (doc.containsKey("cryptoSymbol")) module["cryptoSymbol"] = sanitize(doc["cryptoSymbol"]);
            if (doc.containsKey("cryptoName")) module["cryptoName"] = sanitize(doc["cryptoName"]);
        } else if (moduleType == "stock") {
            if (doc.containsKey("ticker")) module["ticker"] = sanitize(doc["ticker"]);
            if (doc.containsKey("name")) module["name"] = sanitize(doc["name"]);
        } else if (moduleType == "weather") {
            if (doc.containsKey("location")) module["location"] = sanitize(doc["location"]);
            if (doc.containsKey("latitude")) module["latitude"] = doc["latitude"];
            if (doc.containsKey("longitude")) module["longitude"] = doc["longitude"];
        } else if (moduleType == "custom") {
            if (doc.containsKey("label")) module["label"] = sanitize(doc["label"]);
            if (doc.containsKey("value")) module["value"] = doc["value"];
            if (doc.containsKey("unit")) module["unit"] = sanitize(doc["unit"]);
            if (doc.containsKey("decimals")) module["decimals"] = doc["decimals"];
        } else if (moduleType == "countdown") {
            if (doc.containsKey("label")) module["label"] = sanitize(doc["label"]);
            if (doc.containsKey("targetDate")) module["targetDate"] = doc["targetDate"].as<String>();
            if (doc.containsKey("targetTime")) module["targetTime"] = doc["targetTime"].as<String>();
            if (doc.containsKey("countdownType")) module["countdownType"] = doc["countdownType"].as<String>();
            if (doc.containsKey("decimals")) module["decimals"] = doc["decimals"];
        } else if (moduleType == "http") {
            if (doc.containsKey("label")) module["label"] = sanitize(doc["label"]);
            if (doc.containsKey("url")) module["url"] = cleanControlChars(doc["url"]);
            if (doc.containsKey("jsonPath")) module["jsonPath"] = cleanControlChars(doc["jsonPath"]);
            if (doc.containsKey("unit")) module["unit"] = sanitize(doc["unit"]);
        } else if (moduleType == "transit") {
            if (doc.containsKey("routeName")) module["routeName"] = sanitize(doc["routeName"]);
            if (doc.containsKey("stopName")) module["stopName"] = sanitize(doc["stopName"]);
            if (doc.containsKey("apiUrl")) module["apiUrl"] = cleanControlChars(doc["apiUrl"]);
            if (doc.containsKey("jsonPath")) module["jsonPath"] = cleanControlChars(doc["jsonPath"]);
            if (doc.containsKey("apiKey")) module["apiKey"] = cleanControlChars(doc["apiKey"]);
        } else if (moduleType == "quad") {
            // Quad slots contain module IDs - don't sanitize (they're not user input)
            if (doc.containsKey("slot1")) module["slot1"] = doc["slot1"].as<String>();
            if (doc.containsKey("slot2")) module["slot2"] = doc["slot2"].as<String>();
            if (doc.containsKey("slot3")) module["slot3"] = doc["slot3"].as<String>();
            if (doc.containsKey("slot4")) module["slot4"] = doc["slot4"].as<String>();
        }

        // Update refresh interval (with validation) - applies to all types
        if (doc.containsKey("refreshInterval")) {
            uint16_t refreshInterval = doc["refreshInterval"] | 300;
            if (refreshInterval < 10) refreshInterval = 10;  // Min 10s
            if (refreshInterval > 3600) refreshInterval = 3600;  // Max 1 hour
            module["refreshInterval"] = refreshInterval;
        }

        // Save configuration
        saveConfiguration(true);

        // Trigger fetch to update display
        extern Scheduler scheduler;
        scheduler.requestFetch(moduleId.c_str(), true);

        server->send(200, "application/json", "{\"success\":true}");
    });

    // GET /api/module-types - Get available module types
    server->on("/api/module-types", HTTP_GET, [this]() {
        String token = server->header("Authorization");
        if (!security.validateSession(token)) {
            server->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
            return;
        }

        String response = "{\"types\":[";
        response += "{\"id\":\"crypto\",\"name\":\"Cryptocurrency\",\"icon\":\"₿\"},";
        response += "{\"id\":\"stock\",\"name\":\"Stock Price\",\"icon\":\"📈\"},";
        response += "{\"id\":\"weather\",\"name\":\"Weather\",\"icon\":\"🌤\"},";
        response += "{\"id\":\"custom\",\"name\":\"Custom Value\",\"icon\":\"⚡\"},";
        response += "{\"id\":\"countdown\",\"name\":\"Countdown Timer\",\"icon\":\"⏰\"},";
        response += "{\"id\":\"http\",\"name\":\"HTTP/API Fetch\",\"icon\":\"🌐\"},";
        response += "{\"id\":\"transit\",\"name\":\"Public Transit\",\"icon\":\"🚌\"},";
        response += "{\"id\":\"quad\",\"name\":\"Quad Screen\",\"icon\":\"🔲\"}";
        response += "]}";

        server->send(200, "application/json", response);
    });

    Serial.println("Registered module management API endpoints");
    Serial.println("  - GET    /api/modules");
    Serial.println("  - POST   /api/modules");
    Serial.println("  - POST   /api/modules/delete");
    Serial.println("  - POST   /api/modules/order");
    Serial.println("  - POST   /api/modules/update");
    Serial.println("  - GET    /api/module-types");

    Serial.println("Starting WebServer...");
    server->begin();
    Serial.println("✓ WebServer started and listening on port 80");
    Serial.println("=== Settings Server Setup Complete ===\n");
}

// Settings page handlers
void NetworkManager::handleSettingsRoot() {
    server->send_P(200, "text/html", SETTINGS_HTML);
}

void NetworkManager::handleValidateCode() {
    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"valid\":false,\"error\":\"No data\"}");
        return;
    }

    String body = server->arg("plain");
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
        server->send(400, "application/json", "{\"valid\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    uint32_t enteredCode = doc["code"];

    // Check if locked out
    if (security.isLockedOut()) {
        unsigned long remaining = security.getLockoutTimeRemaining();
        char response[128];
        snprintf(response, sizeof(response),
                 "{\"valid\":false,\"error\":\"Locked out for %lu seconds\"}",
                 remaining / 1000);
        server->send(403, "application/json", response);
        return;
    }

    // Validate code
    if (security.validateCode(enteredCode)) {
        // Create session
        String token = security.createSession();

        char response[128];
        snprintf(response, sizeof(response),
                 "{\"valid\":true,\"token\":\"%s\"}", token.c_str());
        server->send(200, "application/json", response);
    } else {
        server->send(200, "application/json", "{\"valid\":false,\"error\":\"Invalid or expired code\"}");
    }
}

void NetworkManager::handleGetConfig() {
    // Check authorization
    String token = server->header("Authorization");
    if (!security.validateSession(token)) {
        server->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }

    // Serialize current configuration
    String response;
    serializeJson(config, response);
    server->send(200, "application/json", response);
}

void NetworkManager::handleUpdateConfig() {
    // Check authorization
    String token = server->header("Authorization");
    if (!security.validateSession(token)) {
        server->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }

    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"No data\"}");
        return;
    }

    String body = server->arg("plain");
    lastPostBody = body;  // Store for debug endpoint

    Serial.println("\n=== RECEIVED CONFIG UPDATE ===");
    Serial.print("Body length: ");
    Serial.println(body.length());
    Serial.println("Body content:");
    Serial.println(body);
    Serial.println("==============================\n");

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
        String errorMsg = "{\"success\":false,\"error\":\"Invalid JSON: ";
        errorMsg += error.c_str();
        errorMsg += "\"}";
        server->send(400, "application/json", errorMsg);
        return;
    }

    // Check if document overflowed
    Serial.print("JSON parse successful. Memory usage: ");
    Serial.print(doc.memoryUsage());
    Serial.print(" / ");
    Serial.print(doc.capacity());
    Serial.println(" bytes");
    if (doc.overflowed()) {
        Serial.println("ERROR: JSON document OVERFLOWED! Data may be lost!");
        server->send(400, "application/json", "{\"success\":false,\"error\":\"Config too large\"}");
        return;
    }

    // Update configuration
    String previousActiveModule = config["device"]["activeModule"] | "bitcoin";
    if (doc.containsKey("device")) {
        if (doc["device"].containsKey("activeModule")) {
            String newActiveModule = doc["device"]["activeModule"].as<String>();
            config["device"]["activeModule"] = newActiveModule;
            // If activeModule changed, mark it for forced fetch
            if (newActiveModule != previousActiveModule) {
                // Clear lastUpdate to trigger immediate fetch
                JsonObject moduleData = config["modules"][newActiveModule];
                moduleData["lastUpdate"] = 0;
                Serial.print("Active module changed to: ");
                Serial.println(newActiveModule);
            }
        }
    }

    if (doc.containsKey("modules")) {
        JsonObject modules = doc["modules"];

        // Update bitcoin crypto config
        if (modules.containsKey("bitcoin")) {
            bool cryptoChanged = false;
            if (modules["bitcoin"].containsKey("cryptoId")) {
                String newId = modules["bitcoin"]["cryptoId"].as<String>();
                String oldId = config["modules"]["bitcoin"]["cryptoId"] | "bitcoin";
                if (newId != oldId) {
                    cryptoChanged = true;
                }
                config["modules"]["bitcoin"]["cryptoId"] = newId;
                Serial.print("Bitcoin cryptoId updated to: ");
                Serial.println(newId);
            }
            if (modules["bitcoin"].containsKey("cryptoSymbol")) {
                String symbol = modules["bitcoin"]["cryptoSymbol"].as<String>();
                config["modules"]["bitcoin"]["cryptoSymbol"] = symbol;
                Serial.print("Bitcoin cryptoSymbol updated to: ");
                Serial.println(symbol);
            }
            if (modules["bitcoin"].containsKey("cryptoName")) {
                String name = modules["bitcoin"]["cryptoName"].as<String>();
                config["modules"]["bitcoin"]["cryptoName"] = name;
                Serial.print("Bitcoin cryptoName updated to: ");
                Serial.println(name);
            }
            // Clear cached data if crypto changed
            if (cryptoChanged) {
                config["modules"]["bitcoin"]["value"] = 0.0;
                config["modules"]["bitcoin"]["change24h"] = 0.0;
                config["modules"]["bitcoin"]["lastUpdate"] = 0;
                Serial.println("Bitcoin crypto changed - cleared cache");
            }
        }

        // Update ethereum crypto config
        if (modules.containsKey("ethereum")) {
            bool cryptoChanged = false;
            if (modules["ethereum"].containsKey("cryptoId")) {
                String newId = modules["ethereum"]["cryptoId"].as<String>();
                String oldId = config["modules"]["ethereum"]["cryptoId"] | "ethereum";
                if (newId != oldId) {
                    cryptoChanged = true;
                }
                config["modules"]["ethereum"]["cryptoId"] = newId;
            }
            if (modules["ethereum"].containsKey("cryptoSymbol")) {
                config["modules"]["ethereum"]["cryptoSymbol"] = modules["ethereum"]["cryptoSymbol"].as<String>();
            }
            if (modules["ethereum"].containsKey("cryptoName")) {
                config["modules"]["ethereum"]["cryptoName"] = modules["ethereum"]["cryptoName"].as<String>();
            }
            // Clear cached data if crypto changed
            if (cryptoChanged) {
                config["modules"]["ethereum"]["value"] = 0.0;
                config["modules"]["ethereum"]["change24h"] = 0.0;
                config["modules"]["ethereum"]["lastUpdate"] = 0;
                Serial.println("Ethereum crypto changed - cleared cache");
            }
        }

        // Update stock config
        if (modules.containsKey("stock")) {
            if (modules["stock"].containsKey("ticker")) {
                config["modules"]["stock"]["ticker"] = modules["stock"]["ticker"].as<String>();
                // Clear cached stock data to force fresh fetch
                config["modules"]["stock"]["value"] = 0.0;
                config["modules"]["stock"]["change"] = 0.0;
                config["modules"]["stock"]["lastUpdate"] = 0;
                config["modules"]["stock"]["lastSuccess"] = false;
            }
            if (modules["stock"].containsKey("name")) {
                config["modules"]["stock"]["name"] = modules["stock"]["name"].as<String>();
            }
        }

        // Update weather config (wttr.in only needs location text, no lat/lon)
        if (modules.containsKey("weather")) {
            Serial.println("=== Processing weather config update ===");

            if (modules["weather"].containsKey("location")) {
                String location = modules["weather"]["location"].as<String>();
                config["modules"]["weather"]["location"] = location;
                Serial.print("Weather location set to: ");
                Serial.println(location);
            } else {
                Serial.println("WARNING: No location in weather update request!");
            }

            // Clear cached weather data to force fresh fetch
            config["modules"]["weather"]["temperature"] = 0;
            config["modules"]["weather"]["condition"] = "Unknown";
            config["modules"]["weather"]["lastUpdate"] = 0;
            config["modules"]["weather"]["lastSuccess"] = false;

            Serial.println("Weather cache cleared, will fetch on next cycle");
        }

        // Update custom config
        if (modules.containsKey("custom")) {
            if (modules["custom"].containsKey("mode")) {
                config["modules"]["custom"]["mode"] = modules["custom"]["mode"].as<String>();
            }
            if (modules["custom"].containsKey("label")) {
                config["modules"]["custom"]["label"] = modules["custom"]["label"].as<String>();
            }
            if (modules["custom"].containsKey("value")) {
                config["modules"]["custom"]["value"] = modules["custom"]["value"].as<float>();
            }
            if (modules["custom"].containsKey("unit")) {
                config["modules"]["custom"]["unit"] = modules["custom"]["unit"].as<String>();
            }
            if (modules["custom"].containsKey("targetDate")) {
                config["modules"]["custom"]["targetDate"] = modules["custom"]["targetDate"].as<String>();
            }
            if (modules["custom"].containsKey("targetTime")) {
                config["modules"]["custom"]["targetTime"] = modules["custom"]["targetTime"].as<String>();
            }
            if (modules["custom"].containsKey("countdownType")) {
                config["modules"]["custom"]["countdownType"] = modules["custom"]["countdownType"].as<String>();
            }

            // Trigger fetch for countdown mode
            String mode = modules["custom"]["mode"] | "manual";
            if (mode == "countdown") {
                Serial.println("Requesting forced fetch for custom module (countdown mode)");
                scheduler.requestFetch("custom", true);
            }
        }
    }

    // Save to file (force=true to bypass throttle)
    // Note: We do NOT reload after save - config is already in memory!
    // Reloading could trigger setDefaultConfig() if validation fails, wiping user data!
    saveConfiguration(true);

    // Trigger forced fetches for modules that changed
    if (doc.containsKey("modules")) {
        JsonObject modules = doc["modules"];
        if (modules.containsKey("bitcoin") && modules["bitcoin"].containsKey("cryptoId")) {
            Serial.println("Requesting forced fetch for bitcoin module");
            scheduler.requestFetch("bitcoin", true);
        }
        if (modules.containsKey("ethereum") && modules["ethereum"].containsKey("cryptoId")) {
            Serial.println("Requesting forced fetch for ethereum module");
            scheduler.requestFetch("ethereum", true);
        }
        if (modules.containsKey("stock") && modules["stock"].containsKey("ticker")) {
            Serial.println("Requesting forced fetch for stock module");
            scheduler.requestFetch("stock", true);
        }
        if (modules.containsKey("weather") && modules["weather"].containsKey("location")) {
            Serial.println("Requesting forced fetch for weather module");
            scheduler.requestFetch("weather", true);
        }
    }

    server->send(200, "application/json", "{\"success\":true,\"message\":\"Settings saved. New data will load within 10 seconds.\"}");
}

void NetworkManager::handleStockSearch() {
    if (!server->hasArg("q")) {
        server->send(400, "application/json", "{\"error\":\"Missing query parameter\"}");
        return;
    }

    String query = server->arg("q");
    Serial.print("Stock search query: ");
    Serial.println(query);

    HTTPClient http;
    http.begin("https://query2.finance.yahoo.com/v1/finance/search?q=" + query + "&quotesCount=10&newsCount=0");
    http.addHeader("User-Agent", "Mozilla/5.0");

    int httpCode = http.GET();

    if (httpCode == 200) {
        String payload = http.getString();
        server->send(200, "application/json", payload);
    } else {
        Serial.printf("Stock API error: %d\n", httpCode);
        server->send(httpCode, "application/json", "{\"error\":\"API request failed\"}");
    }

    http.end();
}

void NetworkManager::handleRestart() {
    // Check authorization
    String token = server->header("Authorization");
    if (!security.validateSession(token)) {
        server->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }

    server->send(200, "application/json", "{\"success\":true}");
    delay(500);
    ESP.restart();
}

void NetworkManager::handleFactoryReset() {
    // Check authorization
    String token = server->header("Authorization");
    if (!security.validateSession(token)) {
        server->send(401, "application/json", "{\"error\":\"Unauthorized\"}");
        return;
    }

    server->send(200, "application/json", "{\"success\":true}");
    delay(500);

    Serial.println("Factory reset via web interface");
    LittleFS.format();
    delay(1000);
    ESP.restart();
}
