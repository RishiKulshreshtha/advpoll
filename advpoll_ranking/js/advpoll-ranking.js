/*global Drupal: true, jQuery: true */
/*jslint indent: 2 */

/*
 * Advanced Ranking Poll
 * Handles behavior of Ranking polls.
*/
(function($) {
  'use strict';

  // storing identifiers arrays to enable more than one ranking poll to render
  // properly if more than one is displayed on the same page.
  var ids = [],
  draggable_ids = [],
  totals = {},
  currentIndices = {};

  Drupal.behaviors.advpollModule = {
    attach: function(context, settings) {
      var i,
      len,
      formID,
      value,
      tableDrag;

      $('.advpoll-ranking-draggable:not(".advpoll-processed")').each(function(i) {
        var $this = $(this),
        nid = $this.attr('data-nid');

        if (nid.length > 0) {
          Drupal.advpoll.draggableSetup(nid);
          $this.addClass('advpoll-processed');
          $('td.advpoll-draggable-weight select').css('display', 'none');
        }

      });

      $('.advpoll-ranking-table-wrapper:not(".advpoll-processed")').each(function(i) {
        var $this = $(this),
        nid = $this.attr('data-nid');

        if (nid.length > 0) {
          Drupal.advpoll.rankingSetup(nid);
          $this.addClass('advpoll-processed');
        }

      });

    }

  };

  // namespace
  Drupal.advpoll = Drupal.advpoll || {};

  /**
   * Get rid of irritating tabledrag messages
   */
  Drupal.theme.tableDragChangedWarning = function() {
    return [];
  };

  Drupal.theme.prototype.tableDragIndentation = function() {
    return [];
  };

  Drupal.theme.prototype.tableDragChangedMarker = function() {
    Drupal.advpoll.draggableUpdate();
    Drupal.advpoll.updateRankingTable();
    return [];
  };

  Drupal.advpoll.setup = function(value) {
    ids.push(value);
  };

  Drupal.advpoll.draggableUpdate = function() {
    var i,
    j,
    len,
    draggable_table,
    rows,
    $row;

    for (i = 0, len = draggable_ids.length; i < len; i += 1) {
      draggable_table = $('#advpoll-ranking-draggable-form-' + draggable_ids[i] + ' .advpoll-ranking-draggable');
      rows = $(draggable_table).find('tbody tr').length;

      for (j = 1; j <= rows; j += 1) {
        $row = $('#advpoll-ranking-draggable-form-' + draggable_ids[i] + ' table tbody tr:nth-child(' + j + ')');

        // update the select menu
        $row.find("select option[value='" + (j) + "']").attr('selected', 'selected');

        // remove attributes from the elements that aren't selected
        $row.find("select option[value!='" + (j) + "']").removeAttr('selected');

      }
    }
  };

  /*
   Initialization converts each labeled select item into a list item.
   Also creating add and remove links that enable the list items to be
   moved back and forth between the list and the table
 */
  Drupal.advpoll.rankingInitialize = function(value) {

    var formID, tableID;

    formID = '#advpoll-ranking-form-' + value;
    tableID = '#advpolltable-' + value;
    totals[value] = $(formID + ' ' + tableID + ' tbody tr').length;
    currentIndices[value] = 0;

    // Adjust the DOM for list to table behavior
    $(formID + ' div.form-type-select').wrap('<li class="selectable" />');
    $(formID + ' div.form-type-textfield').wrap('<li class="selectable" />');
    $(formID + ' li').wrapAll('<ul class="selectable-list" />');
    $(formID + ' label').append('<a href="" class="vote add">' + Drupal.t('Add') + ' ></a>');
    $(formID + ' label').append('<a href="" class="vote remove">(x)</a>');
    $(formID + ' ' + tableID).append('<tfoot><tr class="submit-row"><td></td></tr><tr class="message"><td></td></tr></tfoot>');
    $(formID + ' ' + tableID + ' tfoot tr.submit-row td').append($('.advpoll-ranking-wrapper ' + formID + ' .form-submit'));
    $(formID + ' a.remove').css('display', 'none');
    $(formID + ' li.selectable select').css('display', 'none');

    // adding click events to add and remove links
    $(formID + ' li.selectable').each(function(i) {
      var $this = $(this);
      var remove = $this.find('a.remove');
      var add = $this.find('a.add');

      $(remove).bind('click', function() {
        var partner = add;
        $(this).css('display', 'none');
        partner.css('display', 'block');
        $(formID + ' ul.selectable-list').append($this);
        currentIndices[value] -= 1;
        Drupal.advpoll.updateRankingTable();
        return false;
      });

      $(add).bind('click', function() {
        if (totals[value] - currentIndices[value]) {
          var partner = remove;
          $(this).css('display', 'none');
          partner.css('display', 'block');
          $(formID + ' ' + tableID + ' tbody td').eq(currentIndices[value]).append($this);
          currentIndices[value] += 1;
          Drupal.advpoll.updateRankingTable();
        }
        return false;
      });

    });
  };

  /*
   * Update ranking table so that if there are items removed, items reorder
   * properly into the available rows if one is removed.
   */
  Drupal.advpoll.updateRankingTable = function() {

    for (var i = 0, len = ids.length; i < len; i += 1) {
      var value = ids[i];
      var formID = '#advpoll-ranking-form-' + value;
      var tableID = '#advpolltable-' + value;
      var row_count = $(tableID + ' td.advpoll-weight').length;
      var votes = totals[value] - currentIndices[value];

      // clear all select lists that are not currently in the table.
      $(formID + ' li.selectable').each(function(j) {
        $(this).find("select option[value!='" + (j + 1) + "']").removeAttr('selected');
      });

      // cycle through list items that have been added to the table
      $(tableID + ' td.advpoll-weight li').each(function(j) {
        var li = $(this);
        // make sure the value in the select list matches the index of the list item
        li.find("select option[value='" + (j + 1) + "']").attr('selected', 'selected');
        $(tableID + ' td.advpoll-weight').each(function(k) {
          var td = $(this);
          // the indexes match, so we'll move the li to its matching td index to
          // ensure that it visually appears to be in the correct position in
          // the table
          if (k == j) {
            td.append(li);
          }
        });

      });
      
      if (votes < 1) {
        $(formID + ' ul.selectable-list li.selectable label a.add').css('display', 'none');
      }

      // update counter in table footer
      $(formID + ' ' + tableID + ' tfoot tr.message td').empty().append('<p>' + Drupal.t('Votes remaining: ') + ' ' + votes + '</p>');

    }
  }

  Drupal.advpoll.rankingSetup = function(value) {
    ids.push(value);
    Drupal.advpoll.rankingInitialize(value);
    Drupal.advpoll.updateRankingTable();
  }

  Drupal.advpoll.draggableSetup = function(value) {
    draggable_ids.push(value);
    Drupal.advpoll.draggableUpdate();
  };

}(jQuery));
