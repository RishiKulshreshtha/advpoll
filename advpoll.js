// $Id$

if (Drupal.jsEnabled) {
  $(document).ready(function() {  
    // Update maxchoices, called when adding and removing choices
    function maxChoices(newChoiceN) {
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
      
      $(newMaxChoices).insertAfter(label);
    }
    
    // Click event for Remove link, called on pageload and when Add choice is clicked
    function removeChoiceClick() {
      $('a.remove-choice').unclick().click(function() {
        var nextRemoveLink = $(this).parent().next().find(".remove-choice");
        // Set focus at next or previous Remove link
        if(nextRemoveLink.html()) {
          nextRemoveLink[0].focus();
        }
        else {
          $(this).parent().prev().find(".remove-choice")[0].focus();
        }
        // Remove choice
        $(this).parent().remove();
        var i = 1;
        $("input.choices").prev().each(function() {
          // Give each label it's correct number
          $(this).html($(this).html().replace(/\d+(?=<)/g, i++));
        });
        
        maxChoices(i-1);
        
        return false;
      });
    }
    
    // Hide "need more choices" checkbox
    $("#morechoices").hide();
    
    // Insert Remove links
    $('<a class="remove-choice" href="#">' + Drupal.settings.advPoll.remove + '</a>').insertAfter("input.choices");
    removeChoiceClick();
    
    // "Backup" of the first choice
    var newChoice = $("input.choices:first").parent().clone();
    
    $('<a class="add-choice" href="#">' + Drupal.settings.advPoll.addChoice + '</a>').insertAfter("#morechoices").click(function() {
      var newChoiceN = $("input.choices").length + 1;
      // If all choices are removed, use a "backup" of the first choice, else clone the first.
      newChoice = ($("input.choices:first").parent().html() ? $("input.choices:first").parent().clone() : newChoice);
      // Replace choice numbers in label, name and id with the new choice number
      newChoice.html(newChoice.html().replace(/\d+(?=<)|\d+(?=-)|\d+(?=\])/g, newChoiceN));
      // Clear the value, insert and fade in.
      newChoice.find("input").val("").end().insertBefore("#morechoices").fadeIn();
      // Update hidden form values
      $("#edit-choices").val(newChoiceN);
      $("#edit-changed").val($("#edit-changed").val() + 1);
      
      removeChoiceClick();
      
      maxChoices(newChoiceN);
      
      return false;
    });
  });
}
