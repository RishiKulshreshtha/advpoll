/*global Drupal: true, jQuery: true */
/*jslint devel: true, browser: true, sloppy: true, nomen: true, maxerr: 50, indent: 2 */

/* 
 * Advanced Ranking Poll
 * Handles behavior of Ranking polls.
 http://groups.drupal.org/node/237188#comment-772138
*/
(function ($) {
  // storing identifiers arrays to enable more than one ranking poll to render 
  // properly if more than one is displayed on the same page.  
  var ids          = [],
    draggable_ids  = [],
    totals         = {},
    currentIndices = {};

  Drupal.behaviors.advpollModule = {
    attach: function (context, settings) {
      var i,
        len,
        formID,
        value,
        tableDrag;

      if (Drupal.settings) {
        for (i = 0, len = ids.length; i < len; i += 1) {

          formID = '#advpoll-ranking-form-' + ids[i];
          value = ids[i];

          // only rebuild draggable form if it doesn't yet exist'
          if ($(formID + ' ul.selectable-list').length < 1) {
            currentIndices[ids[i]] = 0;
            $(formID + ' #advpolltable').css('display', 'block');
            $(formID + ' select').css('display', 'none');
            $(formID + ' input.form-text').css('visibility', 'hidden');
            $(formID + ' label').append('<a href="" class="vote add">' + Drupal.t('Add') + ' ></a>');
            $(formID + ' .form-type-select').wrap('<li class="selectable" />');
            $(formID + ' .form-type-textfield').wrap('<li class="selectable" />');
            $(formID + ' li').wrapAll('<ul class="selectable-list" />');
            $(formID + ' #advpolltable').append('<tfoot><tr class="submit-row"><td></td></tr><tr class="message"><td></td></tr></tfoot>');
            $(formID + ' #advpolltable tfoot tr.submit-row td').append($('.advpoll-ranking-wrapper ' + formID + ' .form-submit'));
            $(formID + ' li a').each(function (index) {
              $(this).data('index', index);
            });
            $('#advpolltable tbody td').css('display', 'block');
            $('#advpolltable tr').css('visibility', 'visible');
            // how many possible choices can user make?
            totals[ids[i]] = $(formID + ' #advpolltable tbody tr').length;

            // these calls are checking to see if form has been returned with selected
            // values - need to place them back into the list in the right order if so.
            $(formID + ' ul.selectable-list li').each(function (index) {
              var select,
                selected,
                element,
                input;

              if ($(this).find('select').length) {
                select = $(this).find('select');
                selected = $(".advpoll-ranking-wrapper " + formID + " #" + select.attr('id') + " option[selected = selected]");

                if (select && selected.length) {
                  if (selected.val() > 0) {
                    element = $(selected).parents('li');
                    $(element).find('a.vote').removeClass('add').addClass('remove').html('(x)');
                    $(formID + ' #advpolltable tbody td').eq(selected.val() - 1).append(element);
                    $(formID + ' #advpolltable tbody tr').eq(selected.val() - 1).css('visibility', 'visible');
                    currentIndices[ids[i]] += 1;
                  }
                }
              } else {

                input = $(formID + " input[name='write_in_weight']").attr('value');

                if (input > 0) {
                  element = $(formID + ' ul.selectable-list li .form-item-write-in');

                  element = element.parents('li');
                  $(element).find('a.vote').removeClass('add').addClass('remove').html('(x)');
                  $(formID + ' #advpolltable tbody td').eq(input - 1).append(element);
                  $(formID + ' #advpolltable tbody tr').eq(input - 1).css('visibility', 'visible');
                  currentIndices[ids[i]] += 1;

                }
              }
            });

            Drupal.advpollUpdateEvents(ids[i]);

          }

          if (Drupal.tableDrag) {
            tableDrag = Drupal.tableDrag.advpolltable;

            if (tableDrag.row) {
              // Add a handler for when a row is swapped.
              tableDrag.row.prototype.onSwap = function (swappedRow) {
              };
            }

            // Add a handler so when a row is dropped, update fields dropped into new regions.
            tableDrag.onDrop = function () {
              Drupal.advpollUpdateSelect(value);
              return true;
            };

          }
        }
      }
    }

  };

  // called when an item is added or removed from the list or upon initialization
  Drupal.advpollUpdateEvents =  function (value) {
    var formID = '#advpoll-ranking-form-' + value;
    Drupal.advpollRemoveEvents(value);
    $(formID + ' ul.selectable-list li a.add').bind('click', function () {
      var element = $(this).parents('li');
      if (currentIndices[value] < totals[value]) {
        $(this).removeClass('add').addClass('remove').html('(x)');
      }
      $(formID + ' #advpolltable tbody td').eq(currentIndices[value]).append(element).css('display', 'block');
      $(formID + ' #advpolltable tbody tr').eq(currentIndices[value]).css('visibility', 'visible');
      currentIndices[value] += 1;
      Drupal.advpollUpdateEvents(value);
      return false;
    });


    $(formID + ' td a.remove').bind('click', function () {
      var element = $(this).parents('li'),
        select    = element.find('select');

      $(formID + ' ul.selectable-list').append(element);
      $(this).removeClass('remove').addClass('add').html(Drupal.t('Add >'));

      $(formID + " #" + select.attr('id') + " option[value='0']").attr('selected', 'selected');
      currentIndices[value] -= 1;
      // items are removed so reweight them in the list.
      Drupal.advpollReorderChoices(value);
      Drupal.advpollUpdateEvents(value);
      return false;
    });
    Drupal.advpollUpdateCount(value);

  };

  // called when items are dragged in selected list.
  Drupal.advpollReorderChoices = function (value) {
    var formID = '#advpoll-ranking-form-' + value,
      choices = [],
      i,
      len;

    $(formID + ' tr li').each(function () {
      choices.push($(this));
    });

    for (i = 0, len = choices.length; i < len; i += 1) {
      $(formID + ' #advpolltable tbody td').eq(i).append(choices[i]);
    }
  };

  // Called to ensure that we never bind the click events multiple times.
  Drupal.advpollRemoveEvents =  function (value) {
    var formID = '#advpoll-ranking-form-' + value;

    $(formID + ' ul.selectable-list li a.add').unbind('click');
    $(formID + ' td a.remove').unbind('click');

    Drupal.advpollUpdateSelect(value);
  };

  // Update markup and field values when items are rearranged.
  Drupal.advpollUpdateSelect = function (value) {
    var formID = '#advpoll-ranking-form-' + value;
    $(formID + ' #advpolltable tbody tr').each(function (index) {
      var select;

      if ($(this).find('select').length) {
        $(this).css('visibility', 'visible');
        select = $(this).find('select');
        $(formID + " #" + select.attr('id') + " option[value='" + (index + 1) + "']").attr('selected', 'selected');
      } else if ($(this).find('input').length) {
        $(this).css('visibility', 'visible');
        $(this).find('input').css({
          visibility: 'visible',
          position: 'relative'
        });
        $(formID + " input[name='write_in_weight']").attr('value', (index + 1));
      } else {
        $(this).css('visibility', 'hidden');
      }
    });

    if ($(formID + ' ul.selectable-list li input').length) {
      $(formID + ' ul.selectable-list li input').css({
        visibility: 'hidden',
        position: 'absolute'
      });
      $(formID + " input[name='write_in_weight']").attr('value', 0);
    }

    // if the user has selected the maximum number of votes, hide the add buttons
    if ($(formID + ' #advpolltable tbody li').length >= totals[value]) {
      $(formID + ' ul.selectable-list li a').css('visibility', 'hidden');
    } else {
      $(formID + ' ul.selectable-list li a').css('visibility', 'visible');
    }
  };

  Drupal.advpollUpdateCount = function (value) {
    var formID = '#advpoll-ranking-form-' + value,
      votes    = totals[value] - currentIndices[value];

    $(formID + ' #advpolltable tfoot tr.message td').empty().append('<p>' + Drupal.t('Votes remaining:') + ' ' + votes + '</p>');
  };

  /**
   * Get rid of irritating tabledrag messages
   */
  Drupal.theme.tableDragChangedWarning = function () {
    return [];
  };

  Drupal.theme.prototype.tableDragIndentation = function () {
    return [];
  };

  Drupal.theme.prototype.tableDragChangedMarker = function () {
    Drupal.advpollDraggableUpdate();
    return [];
  };

  Drupal.advpollSetup = function (value) {
    ids.push(value);
  };

  Drupal.advpollDraggableUpdate = function () {
    var i,
      j,
      len,
      draggable_table,
      rows;

    for (i = 0, len = draggable_ids.length; i < len; i += 1) {
      draggable_table = $('#advpoll-ranking-draggable-form-' + draggable_ids[i] + ' #advpoll-ranking-draggable');
      rows = $(draggable_table).find('tbody tr').length;

      for (j = 1; j <= rows; j += 1) {
        $("#advpoll-ranking-draggable-form-" + draggable_ids[i] + " table tbody tr:nth-child(" + j + ") select option[value='" + (j) + "']").attr('selected', 'selected');
      }
    }
  };

  Drupal.advpollDraggableSetup = function (value) {
    draggable_ids.push(value);
    Drupal.advpollDraggableUpdate();
  };

}(jQuery));
