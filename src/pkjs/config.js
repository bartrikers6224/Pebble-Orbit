module.exports = [
  {
    type: 'heading',
    defaultValue: 'Orbit'
  },
  {
    type: 'text',
    defaultValue: 'Three rings around digital time. Hours on the outside, minutes in the middle, seconds inside.'
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Colors'
      },
      {
        type: 'text',
        id: 'SchemePreviews',
        defaultValue:
          '<style>' +
          '.orbit-schemes{display:flex;justify-content:space-around;gap:8px;padding:4px 0 8px;}' +
          '.orbit-scheme{flex:1;background:none;border:0;padding:0;color:#c8c8c8;font:12px/1.2 -apple-system,sans-serif;}' +
          '.orbit-disc{width:84px;height:84px;margin:0 auto 8px;border-radius:50%;position:relative;box-shadow:0 0 0 2px #555;}' +
          '.orbit-scheme.is-selected .orbit-disc{box-shadow:0 0 0 3px #ff4700;}' +
          '.orbit-ring{position:absolute;border-radius:50%;border-style:solid;box-sizing:border-box;}' +
          '.orbit-core{position:absolute;left:50%;top:50%;width:8px;height:8px;margin:-4px 0 0 -4px;border-radius:50%;}' +
          '</style>' +
          '<div class="orbit-schemes">' +
            '<button type="button" class="orbit-scheme" data-scheme="midnight">' +
              '<div class="orbit-disc" style="background:#000">' +
                '<span class="orbit-ring" style="inset:5px;border-width:7px;border-color:#55aaff"></span>' +
                '<span class="orbit-ring" style="inset:16px;border-width:7px;border-color:#00aaff"></span>' +
                '<span class="orbit-ring" style="inset:27px;border-width:7px;border-color:#0055ff"></span>' +
                '<span class="orbit-core" style="background:#fff"></span>' +
              '</div>Midnight</button>' +
            '<button type="button" class="orbit-scheme" data-scheme="dusk">' +
              '<div class="orbit-disc" style="background:#000">' +
                '<span class="orbit-ring" style="inset:5px;border-width:7px;border-color:#aa55ff"></span>' +
                '<span class="orbit-ring" style="inset:16px;border-width:7px;border-color:#ff55aa"></span>' +
                '<span class="orbit-ring" style="inset:27px;border-width:7px;border-color:#ffaa55"></span>' +
                '<span class="orbit-core" style="background:#fff"></span>' +
              '</div>Dusk</button>' +
            '<button type="button" class="orbit-scheme" data-scheme="paper">' +
              '<div class="orbit-disc" style="background:#fff">' +
                '<span class="orbit-ring" style="inset:5px;border-width:7px;border-color:#0055aa"></span>' +
                '<span class="orbit-ring" style="inset:16px;border-width:7px;border-color:#00aaaa"></span>' +
                '<span class="orbit-ring" style="inset:27px;border-width:7px;border-color:#ff5555"></span>' +
                '<span class="orbit-core" style="background:#000"></span>' +
              '</div>Paper</button>' +
          '</div>'
      },
      {
        type: 'text',
        defaultValue: 'Tap a circle to load that look, tweak below if you want, then Save.'
      },
      {
        type: 'color',
        messageKey: 'BackgroundColor',
        defaultValue: '0x000000',
        label: 'Background',
        sunlight: true
      },
      {
        type: 'color',
        messageKey: 'TimeColor',
        defaultValue: '0xFFFFFF',
        label: 'Time',
        sunlight: true
      },
      {
        type: 'color',
        messageKey: 'HoursColor',
        defaultValue: '0x00AAFF',
        label: 'Hours ring',
        sunlight: true
      },
      {
        type: 'color',
        messageKey: 'MinutesColor',
        defaultValue: '0xFF55AA',
        label: 'Minutes ring',
        sunlight: true
      },
      {
        type: 'color',
        messageKey: 'SecondsColor',
        defaultValue: '0x55FF00',
        label: 'Seconds ring',
        sunlight: true
      }
    ]
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Seconds'
      },
      {
        type: 'select',
        messageKey: 'SecondsInterval',
        defaultValue: '10',
        label: 'Update every',
        options: [
          { label: '1 second', value: '1' },
          { label: '10 seconds', value: '10' },
          { label: '30 seconds', value: '30' }
        ]
      },
      {
        type: 'text',
        defaultValue: 'Finer updates use more battery while the watch is awake. With Sleep when idle on, a 1-second ring is reasonable.'
      }
    ]
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Battery'
      },
      {
        type: 'toggle',
        messageKey: 'BatterySave',
        label: 'Sleep when idle',
        defaultValue: false
      },
      {
        type: 'text',
        defaultValue: 'Stops all Orbit updates while charging, or after about five minutes with no motion. The screen goes black so a frozen time is not mistaken for the current time. Shake or tap the watch to wake. Off-wrist is inferred from stillness, not a worn sensor.'
      }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save'
  }
];
