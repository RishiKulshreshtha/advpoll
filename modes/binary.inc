<?php
// $Id$

function advpoll_info_binary() {
  return array(
    'name' => 'binary',
    'name_label' => t('Binary'),
    'description' => t('Vote for or against a number of choices.'),
  );
}

function advpoll_algorithms_binary() {
  return array('plurality' => t('Plurality'));
}

/**
 * Implementation of the view_voting hook for the poll module.
 * 
 * This creates a list of choices to allow the user to vote on choices.
 */
function advpoll_voting_binary_form(&$node, $teaser, $page, $status) {
  static $binary_form_count = 0; 
  $form['#id'] = 'advpoll_voting_binary_form-'. $binary_form_count++;
  $form['#attributes'] = array('class' => 'advpoll-vote');
  
  $form['ajax'] = array(
    '#type' => 'hidden',
    '#attributes' => array('class' => 'ajax'),
  );

  if ($node->choice) {
    $list = array();
    // If previewing check the format against the current users permissions.
    $check = $node->in_preview;
    foreach ($node->choice as $i => $choice) {
      // Don't show blank choices
      if ($choice['label']) {
         $list[$i] = _advpoll_choice_markup($choice['label'], $node->format, $check);
      }
    }
    $form['choice'] = array(
      '#options' => $list,
    );

    if ($node->in_preview) {
      $maxchoices = $node->settings['maxchoices'];
    }
    else {
      $maxchoices = $node->maxchoices; 
    }
    if ($maxchoices == 1) {
      // Plurality voting
      $form['choice']['#type'] = 'radios';
      $form['choice']['#default_value'] = -1;
    }
    else {
      // Approval voting
      $form['choice']['#type'] = 'checkboxes';
    }
  }

  $form['nid'] = array(
    '#type' => 'hidden',
    '#value' => $node->nid,
    '#attributes' => array('class' => 'edit-nid'),
  );

  // Hide vote button if user can't vote and instead display appropriate message.
  if (!$node->in_preview && advpoll_eligible($node) && $status == 'open') {
    static $binary_vote_count = 0;
    $form['vote'] = array(
      '#type' => 'submit',
      '#value' => t('Vote'),
      '#attributes' => array('id' => 'edit-vote-binary-'. $binary_vote_count++),
    );
  }
  elseif ($node->in_preview) {
    //Display nothing
  }
  elseif ($status == 'pending') {
    $form['message']['#value'] = t('This poll opens @time.', array('@time' => format_date($node->startdate)));
  }
  else {
    global $user;
    $login_message = t('<a href="@login">Login</a> to vote in this poll.', array('@login' => url('user/login', drupal_get_destination())));
    $form['message']['#value'] = $user->uid ? t('You are not eligible to vote in this poll.') : $login_message;
  }

  $form['#action'] = url('node/'. $node->nid);
  
  return $form;
}

function advpoll_view_results_binary($node, $teaser, $page) {
  $content_type = 'advpoll';
  $content_id = $node->nid;

  $results = votingapi_get_voting_results($content_type, $content_id);
  $votes = array();
  foreach ($results as $result) {
    $voteval = $result->tag;
    if ($voteval == '_advpoll') {
      if ($result->function == 'total_votes') {
        $total_votes = $result->value;
      }
    }
    else if (isset($node->choice[$voteval])) {
      if (!$votes[$voteval]) {
        $votes[$voteval] = 0;
      }
      $votes[$voteval] = $result->value;
    }
  }

  if ($node->choice && $total_votes > 0) {
    // Add in any choices that received no votes.
    foreach ($node->choice as $i => $ch) {
      if (!isset($votes[$i])) {
        $votes[$i] = 0;
      }
    }

    // Sort results by votes, descending.
    arsort($votes);

    // Display results for each possible choice
    foreach ($votes as $i => $count) {
      $choice = $node->choice[$i];
      $percentage = round(100 * $votes[$i] / $total_votes, 0);
      $output .= theme('advpoll_bar', _advpoll_choice_markup($choice['label'], $node->format, false), $percentage, format_plural($count, '1 vote', '@count votes'));
    }
  }

  return array('results' => $output, 'votes' => $total_votes);
}

function advpoll_calculate_results_binary(&$results, $votes, $node) {
  $voters = array();
  foreach ($votes as $vote) {
    if ($vote->uid) {
      $key = $vote->uid;
    }
    else {
      $key = $vote->hostname;
    }
    $voters[$key] = TRUE;
  }
  $results['_advpoll'] = array(array('total_votes' => count($voters)));
}

/**
 * Registers the vote as a key for this node using votingapi_set_vote().
 */
function advpoll_voting_binary_form_submit($form_id, $form_values) {
  $vote = array();
  $node = node_load($form_values['nid']);
  if ($node->maxchoices == 1) {
    // Plurality voting
    $temp->value = 1;
    $temp->tag = $form_values['choice'];
    $temp->value_type = 'option';
    $vote[] = $temp;
  }
  else {
    // Approval voting
    foreach ($form_values['choice'] as $choice => $selected) {
      unset($temp);
      $temp->value = $choice;
      if ($selected) {
        $temp->value_type = 'option';
        $temp->tag = $choice;
        $temp->value = 1;
        $vote[] = $temp;
      }
    }
  }

  votingapi_set_vote('advpoll', $form_values['nid'], $vote);
  _advpoll_vote_response($node, $form_values);
}

/**
 * Check if the submitted key exists, just to make sure the form is not bypassed.
 *
 * @returns boolean true if the form is valid
 */
function advpoll_voting_binary_form_validate($form_id, $form_values) {
  $node = node_load($form_values['nid']);
  $ajax = $form_values['ajax'];
  
  // Check if user is eligible to vote
  if (!advpoll_eligible($node)) {
    _advpoll_form_set_error('choice[', t('You are not allowed to vote in this poll.'), $ajax);
  }
  
  // Check if poll is active
  if (!_advpoll_is_active($node)) {
    _advpoll_form_set_error('choice[', t('This poll is closed.'), $ajax);
  }

  // Check if user has already voted
  list($voted, $cancel_vote) = _advpoll_user_voted($node->nid);
  if ($voted) {
    _advpoll_form_set_error('choice[', t('You have already voted in this poll.'), $ajax);
  }

  if ($node->maxchoices == 1) {
    // Plurality voting
    if (!array_key_exists($form_values['choice'], $node->choice)) {
      _advpoll_form_set_error('choice[', t('At least one choice must be selected.'), $ajax);
    }
  }
  else {
    // Approval voting
    $numchoices = 0;
    foreach ($node->choice as $i => $val) {
      // see if the box is checked
      if ($form_values['choice'][$i]) {
        $numchoices++;
      }
    }
  
    // Too many choices ranked
    if ($node->maxchoices != 0 && $numchoices > $node->maxchoices) {
      $message = t('%num choices were selected but only %max are allowed.', array('%num' => $numchoices, '%max' => $node->maxchoices));
      _advpoll_form_set_error('choice[', $message, $ajax);

    }

    // Not enough choices ranked
    $minchoices = 1;
    if ($numchoices < $minchoices) {
      _advpoll_form_set_error('choice[', t('At least one choice must be selected.'), $ajax);
    }
  }
}

/**
 * Render the voting form.
 */
function theme_advpoll_voting_binary_form($form) {
  $message = drupal_render($form['message']);
  
  $output = "<div class=\"poll\">\n";
  $output .= drupal_render($form);
  if ($message) {
    $output .= "<p class=\"message\">$message</p>\n";
  }
  $output .= "</div>\n";
  return $output;
}
