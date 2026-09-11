// Interactive Layout Viewer Engine for ErgoDox (WORK-TYPE-GAME)
document.addEventListener('DOMContentLoaded', () => {
  const data = window.KEYBOARD_DATA;
  if (!data) {
    console.error('KEYBOARD_DATA not found!');
    return;
  }

  // App State
  let currentLayer = 0;
  let activeTourStep = null;
  let selectedKey = null;
  let isDarkMode = false;
  let searchFilter = '';

  // Geometry scale: 1u in SVG coordinates
  const UNIT_SIZE = 52;
  const KEY_GAP = 4;
  const CANVAS_ORIGIN_X = 24;
  const CANVAS_ORIGIN_Y = 20;

  // DOM Elements
  const layerTabsContainer = document.getElementById('layer-tabs');
  const svgCanvas = document.getElementById('keyboard-canvas');
  const tourBanner = document.getElementById('tour-banner');
  const tourStepBadge = document.getElementById('tour-step-badge');
  const tourTitle = document.getElementById('tour-title');
  const tourContent = document.getElementById('tour-content');
  const tourPrevBtn = document.getElementById('tour-prev-btn');
  const tourNextBtn = document.getElementById('tour-next-btn');
  const tourCloseBtn = document.getElementById('tour-close-btn');
  const playTourBtn = document.getElementById('play-tour-btn');
  const settingsBtn = document.getElementById('settings-btn');
  const settingsModal = document.getElementById('settings-modal');
  const closeSettingsBtn = document.getElementById('close-settings-btn');
  const settingsTableBody = document.getElementById('settings-table-body');
  const inspectorDrawer = document.getElementById('inspector-drawer');
  const closeInspectorBtn = document.getElementById('close-inspector-btn');
  const searchInput = document.getElementById('search-input');
  const themeToggleBtn = document.getElementById('theme-toggle-btn');

  // Initialize Layer Tabs
  function initLayerTabs() {
    layerTabsContainer.innerHTML = '';
    data.layers.forEach((layer) => {
      const btn = document.createElement('button');
      btn.className = `layer-tab ${layer.position === currentLayer ? 'active' : ''}`;
      btn.innerHTML = `${layer.title} <span class="layer-idx-pill">${layer.position}</span>`;
      btn.addEventListener('click', () => {
        setLayer(layer.position);
      });
      layerTabsContainer.appendChild(btn);
    });
  }

  // Switch Layer
  function setLayer(layerIndex) {
    currentLayer = layerIndex;
    // Update tabs UI
    const tabs = layerTabsContainer.querySelectorAll('.layer-tab');
    tabs.forEach((tab, idx) => {
      if (idx === layerIndex) {
        tab.classList.add('active');
        tab.scrollIntoView({ behavior: 'smooth', block: 'nearest', inline: 'center' });
      } else {
        tab.classList.remove('active');
      }
    });

    renderKeyboard();

    // If key was selected, re-inspect on new layer
    if (selectedKey !== null) {
      inspectKey(selectedKey);
    }
  }

  // Calculate SVG Key Rectangle & Rotation Center
  function getKeyGeometry(key) {
    const rawW = key.w || 1;
    const rawH = key.h || 1;
    const x = CANVAS_ORIGIN_X + key.x * UNIT_SIZE;
    const y = CANVAS_ORIGIN_Y + key.y * UNIT_SIZE;
    const width = rawW * UNIT_SIZE - KEY_GAP;
    const height = rawH * UNIT_SIZE - KEY_GAP;
    const cx = x + width / 2;
    const cy = y + height / 2;
    const rot = key.r || 0;
    const pivotX = key.rawKey && key.rawKey.rx !== undefined ? CANVAS_ORIGIN_X + key.rawKey.rx * UNIT_SIZE : (key.rx !== undefined ? CANVAS_ORIGIN_X + key.rx * UNIT_SIZE : cx);
    const pivotY = key.rawKey && key.rawKey.ry !== undefined ? CANVAS_ORIGIN_Y + key.rawKey.ry * UNIT_SIZE : (key.ry !== undefined ? CANVAS_ORIGIN_Y + key.ry * UNIT_SIZE : cy);

    return { x, y, width, height, cx, cy, rot, pivotX, pivotY };
  }

  // Render SVG Keyboard Canvas
  function renderKeyboard() {
    svgCanvas.innerHTML = '';

    const currentLayerData = data.layers[currentLayer];
    if (!currentLayerData) return;

    // SVG Defs for drop-shadows and icons
    const defs = document.createElementNS('http://www.w3.org/2000/svg', 'defs');
    defs.innerHTML = `
      <filter id="key-shadow" x="-8%" y="-8%" width="120%" height="120%">
        <feDropShadow dx="0" dy="1.5" stdDeviation="1.5" flood-opacity="0.08" />
      </filter>
    `;
    svgCanvas.appendChild(defs);

    // Center "EZ" Emblem
    const centerLogoG = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    centerLogoG.setAttribute('transform', `translate(${CANVAS_ORIGIN_X + 9.35 * UNIT_SIZE}, ${CANVAS_ORIGIN_Y + 1.2 * UNIT_SIZE})`);
    centerLogoG.innerHTML = `
      <rect width="64" height="42" rx="8" fill="#1e293b" />
      <text x="32" y="26" text-anchor="middle" font-size="18" font-weight="900" fill="#ffffff" letter-spacing="1">EZ</text>
    `;
    svgCanvas.appendChild(centerLogoG);

    // Render 76 keys
    currentLayerData.keys.forEach((k) => {
      const geo = getKeyGeometry(k);
      const g = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      g.setAttribute('class', `key-group type-${k.keyType}`);
      g.dataset.index = k.idx;

      // Handle tour highlight
      if (activeTourStep !== null) {
        const step = data.tourSteps[activeTourStep];
        if (step && step.keyIndices && step.keyIndices.includes(k.idx)) {
          g.classList.add('tour-active');
        }
      }

      // Handle search match
      if (searchFilter) {
        const query = searchFilter.toLowerCase();
        const matches = (k.tapText && k.tapText.toLowerCase().includes(query)) ||
                        (k.subText && k.subText.toLowerCase().includes(query)) ||
                        (k.tapCode && k.tapCode.toLowerCase().includes(query)) ||
                        (k.customLabel && k.customLabel.toLowerCase().includes(query));
        if (matches) {
          g.classList.add('search-match');
        }
      }

      // Rotation Transform if applicable
      if (geo.rot !== 0) {
        g.setAttribute('transform', `rotate(${geo.rot} ${geo.pivotX} ${geo.pivotY})`);
      }

      // Keycap Background Rect
      const rect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
      rect.setAttribute('class', 'key-cap');
      rect.setAttribute('x', geo.x);
      rect.setAttribute('y', geo.y);
      rect.setAttribute('width', geo.width);
      rect.setAttribute('height', geo.height);
      rect.setAttribute('rx', 6);
      rect.setAttribute('filter', 'url(#key-shadow)');
      g.appendChild(rect);

      // Key Text (Tap and Sub text)
      if (k.tapText) {
        const textTap = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        textTap.setAttribute('class', 'key-text-primary');
        const textY = k.subText ? geo.y + geo.height * 0.38 : geo.y + geo.height * 0.52;
        textTap.setAttribute('x', geo.cx);
        textTap.setAttribute('y', textY);

        // Resize font for long tap texts
        if (k.tapText.length > 5) {
          textTap.style.fontSize = '10.5px';
        } else if (k.tapText.length > 3) {
          textTap.style.fontSize = '11.5px';
        }

        textTap.textContent = k.tapText;
        g.appendChild(textTap);
      }

      if (k.subText) {
        const textSub = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        textSub.setAttribute('class', 'key-text-sub');
        textSub.setAttribute('x', geo.cx);
        textSub.setAttribute('y', geo.y + geo.height * 0.74);

        if (k.subText.length > 7) {
          textSub.style.fontSize = '8px';
        }

        // Add special icon symbol if applicable
        let subDisplayText = k.subText;
        if (k.subText.includes('Win')) subDisplayText = '⊞ ' + subDisplayText;
        if (k.subText.includes('Shift')) subDisplayText = '⇧ ' + subDisplayText;

        textSub.textContent = subDisplayText;
        g.appendChild(textSub);
      }

      // Key Click Listener
      g.addEventListener('click', () => {
        inspectKey(k.idx);
      });

      svgCanvas.appendChild(g);
    });
  }

  // Key Inspector
  function inspectKey(keyIdx) {
    selectedKey = keyIdx;
    const currentLayerData = data.layers[currentLayer];
    const key = currentLayerData.keys.find(k => k.idx === keyIdx);
    if (!key) return;

    document.getElementById('drawer-key-title').textContent = `Клавиша #${key.idx} [Слой ${currentLayer}: ${currentLayerData.title}]`;
    document.getElementById('insp-tap-code').textContent = key.tapCode || 'KC_NO';
    document.getElementById('insp-tap-label').textContent = key.tapText || '—';
    document.getElementById('insp-hold-code').textContent = key.holdCode || '—';
    document.getElementById('insp-hold-label').textContent = key.subText || '—';
    document.getElementById('insp-custom-label').textContent = key.customLabel || '—';
    document.getElementById('insp-key-type').textContent = key.keyType.toUpperCase();

    // ZMK Code Equivalent
    let zmkCode = generateZmkCode(key);
    document.getElementById('insp-zmk-code').textContent = zmkCode;

    inspectorDrawer.classList.add('open');
  }

  // Generate ZMK code preview for a key
  function generateZmkCode(k) {
    if (k.keyType === 'mod_tap') {
      const mod = k.subText ? k.subText.replace('Left ', 'L').replace('Right ', 'R').toUpperCase() : 'MOD';
      const code = k.tapCode.replace('KC_', '');
      return `&hml ${mod} ${code}  /* Positional Hold-Tap */`;
    }
    if (k.keyType === 'layer_tap') {
      const layerName = k.subText || 'LAYER';
      const code = k.tapCode.replace('KC_', '');
      return `&lt ${layerName} ${code}`;
    }
    if (k.keyType === 'layer_toggle') {
      return `&tog ${k.subText || 'LAYER'}`;
    }
    if (k.keyType === 'layer_osl') {
      return `&sl ${k.subText || 'LAYER'}  /* Sticky Layer */`;
    }
    if (k.keyType === 'func_off') {
      return `&to 0  /* Сброс в базовый слой WORK */`;
    }
    if (k.keyType === 'bluetooth' || k.keyType === 'dongle' || k.tapCode.startsWith('BT_') || k.tapCode.startsWith('OUT_')) {
      if (k.tapCode === 'BT_SEL_0' || k.tapText === 'Dongle') {
        return `&bt BT_SEL 0  /* Беспроводной USB-донгл (ПК 2.4G) */`;
      }
      if (k.tapCode.startsWith('BT_SEL_')) {
        const slot = parseInt(k.tapCode.replace('BT_SEL_', ''));
        return `&bt BT_SEL ${slot}  /* Bluetooth профиль ${slot} (${k.subText || 'Устройство'}) */`;
      }
      if (k.tapCode === 'BT_CLR') {
        return `&bt BT_CLR  /* Сброс сопряжения активного профиля */`;
      }
      if (k.tapCode === 'OUT_TOG') {
        return `&out OUT_TOG  /* Переключение Dongle (USB) ↔ Bluetooth */`;
      }
      return `&${k.tapCode.toLowerCase()}`;
    }
    if (k.keyType === 'lang' || k.tapCode.startsWith('LC(')) {
      return `&kp ${k.tapCode}  /* Язык: ${k.tapText} (${k.subText}) */`;
    }
    if (k.tapCode === 'KC_TRANSPARENT') {
      return `&trans`;
    }
    return `&kp ${k.tapCode.replace('KC_', '')}`;
  }

  // Tour System
  function startTour() {
    activeTourStep = 0;
    tourBanner.classList.add('active');
    updateTourUI();
  }

  function endTour() {
    activeTourStep = null;
    tourBanner.classList.remove('active');
    renderKeyboard();
  }

  function updateTourUI() {
    if (activeTourStep === null) return;
    const step = data.tourSteps[activeTourStep];
    if (!step) return;

    tourStepBadge.textContent = `Шаг ${step.stepIndex} из ${data.tourSteps.length}`;
    tourTitle.textContent = `${step.title} (${step.layerTitle} ${step.layerPosition})`;
    
    // Parse markdown bold in text
    let formattedText = step.text.replace(/\*\*(.*?)\*\*/g, '<strong>$1</strong>');
    formattedText = formattedText.replace(/\n\n/g, '</p><p>');
    tourContent.innerHTML = `<p>${formattedText}</p>`;

    // Auto-switch layer to step's layer
    if (currentLayer !== step.layerPosition) {
      setLayer(step.layerPosition);
    } else {
      renderKeyboard();
    }

    tourPrevBtn.disabled = (activeTourStep === 0);
    tourNextBtn.textContent = (activeTourStep === data.tourSteps.length - 1) ? 'Завершить тур' : 'Вперед ▶';
  }

  function nextTourStep() {
    if (activeTourStep === null) return;
    if (activeTourStep < data.tourSteps.length - 1) {
      activeTourStep++;
      updateTourUI();
    } else {
      endTour();
    }
  }

  function prevTourStep() {
    if (activeTourStep === null) return;
    if (activeTourStep > 0) {
      activeTourStep--;
      updateTourUI();
    }
  }

  // Initialize Settings Modal
  function initSettingsModal() {
    settingsTableBody.innerHTML = '';
    data.settings.forEach(s => {
      const tr = document.createElement('tr');
      tr.innerHTML = `
        <td><strong>${s.name}</strong></td>
        <td><span class="badge-val">${s.value}</span></td>
        <td style="color: var(--text-muted);">${s.defaultVal}</td>
        <td>${s.desc}</td>
      `;
      settingsTableBody.appendChild(tr);
    });
  }

  // Search Filter
  searchInput.addEventListener('input', (e) => {
    searchFilter = e.target.value.trim();
    renderKeyboard();
  });

  // Event Listeners
  playTourBtn.addEventListener('click', startTour);
  tourCloseBtn.addEventListener('click', endTour);
  tourNextBtn.addEventListener('click', nextTourStep);
  tourPrevBtn.addEventListener('click', prevTourStep);

  settingsBtn.addEventListener('click', () => {
    initSettingsModal();
    settingsModal.classList.add('open');
  });

  closeSettingsBtn.addEventListener('click', () => {
    settingsModal.classList.remove('open');
  });

  closeInspectorBtn.addEventListener('click', () => {
    inspectorDrawer.classList.remove('open');
    selectedKey = null;
  });

  // Dark/Light Theme Toggle
  themeToggleBtn.addEventListener('click', () => {
    isDarkMode = !isDarkMode;
    document.body.setAttribute('data-theme', isDarkMode ? 'dark' : 'light');
    themeToggleBtn.textContent = isDarkMode ? '☀️ Светлая' : '🌙 Тёмная';
    renderKeyboard();
  });

  // Keyboard Shortcuts (Arrows for Tour, Esc for close)
  window.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
      if (settingsModal.classList.contains('open')) {
        settingsModal.classList.remove('open');
      } else if (inspectorDrawer.classList.contains('open')) {
        inspectorDrawer.classList.remove('open');
      } else if (activeTourStep !== null) {
        endTour();
      }
    } else if (activeTourStep !== null) {
      if (e.key === 'ArrowRight') nextTourStep();
      if (e.key === 'ArrowLeft') prevTourStep();
    }
  });

  // Initial Boot
  initLayerTabs();
  setLayer(0);
});
