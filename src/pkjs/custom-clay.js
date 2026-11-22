module.exports = function() {
  var clayConfig = this;

  var schemes = {
    midnight: {
      BackgroundColor: 0x000000,
      TimeColor: 0xFFFFFF,
      HoursColor: 0x55AAFF,
      MinutesColor: 0x00AAFF,
      SecondsColor: 0x0055FF
    },
    dusk: {
      BackgroundColor: 0x000000,
      TimeColor: 0xFFFFFF,
      HoursColor: 0xAA55FF,
      MinutesColor: 0xFF55AA,
      SecondsColor: 0xFFAA55
    },
    paper: {
      BackgroundColor: 0xFFFFFF,
      TimeColor: 0x000000,
      HoursColor: 0x0055AA,
      MinutesColor: 0x00AAAA,
      SecondsColor: 0xFF5555
    }
  };

  function applyScheme(name) {
    var scheme = schemes[name];
    if (!scheme) {
      return;
    }

    var keys = [
      'BackgroundColor',
      'TimeColor',
      'HoursColor',
      'MinutesColor',
      'SecondsColor'
    ];
    for (var i = 0; i < keys.length; i++) {
      var item = clayConfig.getItemByMessageKey(keys[i]);
      if (item) {
        item.set(scheme[keys[i]]);
      }
    }
  }

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    var picker = clayConfig.getItemById('ColorScheme');
    if (!picker) {
      return;
    }
    picker.on('change', function() {
      applyScheme(this.get());
    });
  });
};
