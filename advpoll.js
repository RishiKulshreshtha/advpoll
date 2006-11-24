// $Id$

// Global killswitch
if (Drupal.jsEnabled) {
  $(document).ready(function() {  
    // Hide "need more choices" checkbox
    $("#morechoices").hide();
    $('<a href="javascript:void(0)">' + Drupal.settings.advPoll.addChoice + '</a>').insertAfter("#morechoices").click(function() {
      
      // New choice input field 
      var newChoiceN = $("input.choices").length + 1;
      var newChoice = $("#edit-choice-1-label").parent().clone();
      // Replace choice numbers in label, name and id with the new choice number
      newChoice.html(newChoice.html().replace(/\d+(?=<)|\d+(?=-)|\d+(?=\])/g, newChoiceN));
      // Clear the value, insert and fade in.
      newChoice.find("input").val("").end().insertBefore("#morechoices").fadeIn();
      // Update hidden form values
      $("#edit-choices").val(newChoiceN);
      $("#edit-changed").val($("#edit-changed").val() + 1);
      
      // Maximum choices select field
      var selected = $("#edit-settings-maxchoices").val();
      var label = $("#edit-settings-maxchoices").prev();
      // Hard-code the HTML (not clone) as .html() doesn't work for select fields in IE and Opera.
      var newMaxChoices = '<select id="edit-settings-maxchoices" class="form-select" name="settings[maxchoices]">';
      // Build the options
      for (var i = 0; i <= newChoiceN; i++) {
        var name = (i ? i : Drupal.settings.advPoll.noLimit);
        newMaxChoices += '<option ';
        // Set the option user had selected
        if (i == selected) {
          newMaxChoices += 'selected="selected" ';
        }
        newMaxChoices += 'value="' + i + '">' + name + '</option>';
      }
      newMaxChoices += '</select>';
      // Remove old maxchoices
      $("#edit-settings-maxchoices").remove();
      // Insert new maxchoices after label
      $(newMaxChoices).insertAfter(label);
      
      return false;
    });
  });
}
