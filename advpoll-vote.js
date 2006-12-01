// $Id$

if (typeof(Drupal) == "undefined" || !Drupal.advpoll) {
  Drupal.advpoll = {};
}

/*
* Submit advpoll forms with ajax
*/
Drupal.advpoll.attachVoteAjax = function() {
  var submitSelect = "input[@value=" + Drupal.settings.advPoll.vote +"]";
  $("form.advpoll-vote").each(function() {
    var thisForm = this;
    var options = {
      dataType: 'json',
      after: function(data) {
        // Remove previous messages
        $("div.messages").remove(); 
        
        // Insert response
        if (data.errors) {
          $(data.errors).insertBefore(thisForm).fadeIn();
        }
        else {
          $(thisForm).hide(); 
          // Insert messages last to prevent "blinking" when fading in, hence the extra code
          var response = $(data.response).insertBefore(thisForm)[0];
          $(data.statusMsgs).insertBefore(response).fadeIn();
        }

        // Re-enable the Vote button if there was an error message
        $(submitSelect, thisForm).removeAttr("disabled");

      },

      before: function() {
        // Disable all voting forms for this poll
        var nid = $("input.edit-nid", thisForm).val();
        $("form.advpoll-vote").each(function() {
          if ($("input.edit-nid", this).val() == nid) {
            $(submitSelect, this).attr("disabled", "disabled");
          }
        });
      }
    };
    // Tell the server we are passing the form values with ajax and attach the function
    $("input.ajax", thisForm).val(true);
    $(this).ajaxForm(options);
  });
}

Drupal.advpoll.nodeVoteAutoAttach = function() {
  Drupal.advpoll.attachVoteAjax();
}

if (Drupal.jsEnabled) {
  $(document).ready(function(){  
    Drupal.advpoll.nodeVoteAutoAttach();
  });
};
