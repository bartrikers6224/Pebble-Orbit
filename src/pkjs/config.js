module.exports = [
  {
    type: 'heading',
    defaultValue: 'Orbit'
  },
  {
    type: 'text',
    defaultValue: 'Three rings around digital time. Hours on the outside, minutes in the middle, seconds inside — seconds step every 10 seconds to save battery.'
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
    type: 'submit',
    defaultValue: 'Save'
  }
];
