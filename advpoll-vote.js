// $Id$

if (typeof(Drupal) == "undefined" || !Drupal.advpoll) {
  Drupal.advpoll = {};
}

/*
* Submit advpoll forms with ajax
*/
Drupal.advpoll.attachVoteAjax = function() {
  $("form.advpoll-vote").each(function() {
    var thisForm = this;
    var options = {
      dataType: 'json',
      after: function(data) {
        // Remove previous messages
        $("div.messages").remove(); 
        // Remove the voting form if validating passes
        if (!data.error) {
          $(thisForm).hide(); 
        }
        // Insert the response (voting result or error message)
        $(data.response).insertBefore(thisForm);
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
