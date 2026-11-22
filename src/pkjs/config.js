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
