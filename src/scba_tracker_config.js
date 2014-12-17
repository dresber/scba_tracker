Pebble.addEventListener("ready", 
  function(e) {
    console.log("PebbleKit Js ready");
  }
);

Pebble.addEventListener("showConfiguration",
  function(e) {
    Pebble.openURL("https://www.googledrive.com/host/0B2O_EhizVtu7NUN0QkFUU3RKZHc");
  }
);

Pebble.addEventListener("webviewclosed",
  function(e){
    var configuration = JSON.parse(decodeURIComponent(e.response));
    console.log("Configuration window returned: " + JSON.stringify(configuration));
    
    var dictionary = {
      "SCBA_STORE_KEY_BREATHING_RATE": configuration.breath_rate,
      "SCBA_STORE_KEY_BOTTLE_ONE_AVAILABLE": configuration.type1,
      "SCBA_STORE_KEY_BOTTLE_TWO_AVAILABLE": configuration.type2,
      "SCBA_STORE_KEY_BOTTLE_THREE_AVAILABLE": configuration.type3,
      "SCBA_STORE_KEY_BOTTLE_FOUR_AVAILABLE": configuration.type4,
      "SCBA_STORE_KEY_BOTTLE_FIVE_AVAILABLE": configuration.type5,
      "SCBA_STORE_KEY_BOTTLE_SIX_AVAILABLE": configuration.type6,
      "SCBA_STORE_KEY_DEFAULT_BOTTLE": configuration.def_bottle
    };
    
    Pebble.sendAppMessage(
      dictionary,
      
      function(e){
        console.log("Sending breath_rate...");
      },
      
      function(e){
        console.log("Settings feedback failed...");                             
      }
    );

    Pebble.sendAppMessage(
      {},
      
      function(e){
        console.log("Sending bottle_type1...");
      },
      
      function(e){
        console.log("Settings feedback failed...");                             
      }
    );

  }
);
