<?php
// $Id$

function advpoll_info_ranking() {
  return array(
    'name' => 'ranking',
    'name_label' => t('Ranking'),
    'description' => t('Rank a number of choices.'),
  );
}

function advpoll_algorithms_ranking() {
  return array(
    'borda count' => t('borda count'),
    'instant runoff' => t('instant runoff'),
  );
}

function advpoll_voting_ranking_form(&$node, $teaser, $page) {
  $form = array();

  static $ranking_form_count = 0; 
  $form['#id'] = 'advpoll_voting_ranking_form-'. $ranking_form_count++;
  $form['#attributes'] = array('class' => 'advpoll-vote');

  $form['ajax'] = array(
    '#type' => 'hidden',
    '#attributes' => array('class' => 'ajax'),
  );


  if ($node->choice) {
    $list = array();

    $num_choices = count($node->choice);

    // Generate the list of possible rankings
    $choices[0] = '--';
    for ($i = 1; $i <= $num_choices; $i++) {
      if ($i == 1) {
        $val = t('1st');
      }
      elseif ($i == 2) {
        $val = t('2nd');
      }
      elseif ($i == 3) {
        $val = t('3rd');
      }
      else {
        $val = t($i .'th');
      }
      $choices[$i] = $val;
    }

    $form['choice'] = array(
      '#tree' => TRUE,
      // XXX: Workaround for FormAPI bug in PHP 4, see http://drupal.org/node/86657
      '#type' => 'checkboxes',
    );

    foreach ($node->choice as $key => $choice) {
      // Don't show blank choices
      if ($choice['label']) {
        $form['choice'][$key] = array(
          '#type' => 'select',
          '#title' => _advpoll_choice_markup($choice['label'], $node->format),
          '#options' => $choices,
        );
      }
    }
  }

  $form['nid'] = array(
    '#type' => 'hidden',
    '#value' => $node->nid,
    '#attributes' => array('class' => 'edit-nid'),
  );

  if (!$node->in_preview) {
    static $ranking_vote_count = 0;
    $form['vote'] = array(
      '#type' => 'submit',
      '#value' => t('Vote'),
      '#attributes' => array('id' => 'edit-vote-rank-'. $ranking_vote_count++),
    );
  }

  // Disable button and display message for non-eligible users.
  if (!advpoll_eligible($node)) {
    global $user;
    $form['vote']['#disabled'] = TRUE;
    $login_message = t('<a href="@login">Login</a> to vote in this poll.', array('@login' => url('user/login', drupal_get_destination())));
    $form['vote']['#suffix'] = ($user->uid ? t('You are not eligible to vote in this poll.') : $login_message);
  }
  
  $form['#action'] = url('node/'. $node->nid);
  
  return $form;
}

function advpoll_view_results_ranking($node, $teaser, $page) {
  $results = votingapi_get_voting_results('advpoll', $node->nid);
  $round_table = '';

  // If no one has voted, $results = array() and thus is empty.
  if (!empty($results)) {
    // Temporary list of choices indexes for the ranking
    $ranking_list = array();
    // Result object
    $ranking = array();
    $choices = array();
    $poll = array();
    $rounds = array();
    foreach ($results as $result) {
      $tag = $result->tag;
      if ($tag == '_advpoll') {
        // Poll-wide cached value
        $poll[$result->function] = $result->value;
      }
      else if (strstr($tag, '_rounds_')) {
        // Reconstruct round data.
        // Extract the round from the tag.
        $round = str_replace('_rounds_', '', $tag);
        if (!isset($rounds[$round])) {
          $rounds[$round] = array();
        }
        // $result->function actually stores $choice.
        $rounds[$round][$result->function] = $result->value;
      }
      else if (isset($node->choice[$tag])) {
        // Choice-specific cached value
        if ($result->function == 'ranking') {
          $ranking_list[$result->value][] = $tag;
        }
        else if (!isset($node->choice[$result->function])) {
          $choices[$tag][$result->function] = $result->value;
        }
      }
    }

    // Re-construct the rankings object
    foreach ($ranking_list as $i => $choice_list) {
      $ranking[$i]->choices = array();
      foreach ($choice_list as $choice_i) {
        $ranking[$i]->choices[] = $choice_i;
        $ranking[$i]->viewscore = $choices[$choice_i]['viewscore'];
        $ranking[$i]->rawscore = $choices[$choice_i]['rawscore'];
        if ($choices[$choice_i]['percentage']) {
          $ranking[$i]->percentage = $choices[$choice_i]['percentage'];
        }    
      }
    }

    if ($node->algorithm == 'borda count') {
      for ($i = 0; $i < count($ranking); $i++) {
        $first_one = true;
        $this_rank = '';

        // Loop through all choices with this ranking
        foreach ($ranking[$i]->choices as $choice) {
          $this_rank .= ($first_one? '' : ', ') . _advpoll_choice_markup($node->choice[$choice]['label'], $node->format, false);
          $first_one = false;
        }

        $percentage = round(100 * $ranking[$i]->percentage, 0);
        $output .= theme('advpoll_bar', $this_rank, $percentage, $ranking[$i]->viewscore);

      }
    }
    else {
      $output .= '<ol>';

      for ($i = 0; $i < count($ranking); $i++) {
        $output .= '<li> ';
        $first_one = true;
        // If previewing check the format against the current users permissions.
        $check = $node->in_preview;

        // Loop through all choices with this ranking
        foreach ($ranking[$i]->choices as $choice) {
          $output .= ($first_one? '' : ', ') . _advpoll_choice_markup($node->choice[$choice]['label'], $node->format, $check);
          $first_one = false;
        }

        // Show the ranking's score if it exists (depends on algorithm)
        if ($ranking[$i]->viewscore) {
          $output .= ' ('. $ranking[$i]->viewscore .'%)';
        }
        $output .= '</li>';
      }
    }
    $output .= '</ol>';

    if (user_access('inspect all votes') && isset($rounds)) {
      if (count($rounds) > 0) {
        $header[0] = t('Rounds');
        $total_rounds = count($rounds);
        for ($i = 0; $i < count($rounds); $i++) {
          $choices = $rounds[$i];
          if ($i + 1 == $total_rounds) {
            // This is the last round.
            $header[$i + 1] = t('Final');
          }
          else {
            $header[$i + 1] = $i + 1;
          }
          if ($i == 0) {
            $rows = array();
          }
          foreach ($node->choice as $key => $data) {
            $rows[$key][0] = $data['label'];
            $rows[$key][$i + 1] = isset($choices[$key]) && $choices[$key]? $choices[$key] : '';
          }
        }
        $round_table = theme('table', $header, $rows, array(), t('Per-round breakdown of votes for each choice'));
      }
    }
  }
  $output .= $round_table;
  return array('results' => $output, 'votes' => $poll['total_votes']);
}

/**
 * Calculate the results for a ranking poll based on the algorithm.
 *
 * @param $node
 *  The node object for the current poll
 *
 * @return 
 *   Should return an object that include the following attributes
 *   -results : 2d array listing the aggregate preference, including ties
 *   -rounds : 2d array listing the per-choice vote count for each round and
 *              a status message indicating who was eliminated
 *   -totalVoters : the total number of voters who participated
 */
function advpoll_calculate_results_ranking(&$cache, $votes, $node) {
  if ($node->algorithm == 'borda count') {
    $results = _advpoll_calculate_bordacount($node);
  }
  else {
    $results = _advpoll_calculate_instantrunoff($node);
  }

  // Cache rankings
  // API: $cache[$tag][$type][$function] = $value (0 is the default $type)
  for ($i = 0; $i < count($results->ranking); $i++) {
    foreach ($results->ranking[$i]['choices'] as $choice) {
      $cache[$choice][0]['ranking'] = $i;
      $cache[$choice][0]['rawscore'] = $results->ranking[$i]['rawscore'];
      $cache[$choice][0]['viewscore'] = $results->ranking[$i]['viewscore'];
      if (isset($results->ranking[$i]['percentage'])) {
        $cache[$choice][0]['percentage'] = $results->ranking[$i]['percentage'];
      }
    }
  }

  // Cache round results
  if (isset($results->matrix)) {
    foreach ($results->matrix as $i => $round) {
      $key = '_rounds_'. $i;
      $cache[$key] = array();
      foreach ($round as $choice => $votes) {
        $cache[$key][0][$choice] = count($votes);
      }
    }
  }

  // Cache total votes
  $cache['_advpoll'][0]['total_votes'] = $results->total_votes;

  // Cache total points (if it exists)
  if (isset($results->total_points)) {
    $cache['_advpoll'][0]['total_points'] = $results->total_points;
  }
}

/**
 * Calculate the results using borda count.
 * 
 * @param $node
 *  The node object for the current poll.
 *
 * @return 
 *   Should return an object that include the following attributes
 *   -results : 2d array listing the aggregate preference, including ties
 *   -rounds : 2d array listing the per-choice vote count for each round and
 *              a status message indicating who was eliminated
 *   -totalVoters : the total number of voters who participated
 */
function _advpoll_calculate_bordacount($node) {
   $votes = array();
  // ORDER BY value ASC lets us ensure no gaps
  $result = db_query("SELECT * FROM {votingapi_vote} v WHERE content_type='%s' AND content_id=%d ORDER BY value ASC", 'advpoll', $node->nid);
  while ($vobj = db_fetch_object($result)) {
    $votes[] = $vobj;
  }

  if (count($votes) == 0) {
    // No votes yet
    return array();
  }

  // Aggregate votes by user (uid if logged in, IP if anonymous)
  // in ascending order of value.
  $user_votes = array();

  foreach ($votes as $vote) {
    if ($vote->uid == 0) {
      // Anonymous user
      $key = $vote->hostname;
    }
    else {
      // Logged-in user
      $key = $vote->uid;
    }

    $user_votes[$key][$vote->value] = $vote->tag;
  }

  $choice_votes = array();

  $total_choices = count($node->choice);
  $total_points = 0;

  // Loop through each user's vote
  foreach ($user_votes as $uid => $user_vote) {
    foreach ($user_vote as $ranking => $choice) {
      // Negative values are possible if choices were removed after vote
      $vote_value = max($total_choices - $ranking, 0);
      $choice_votes[$choice] +=   $vote_value;
      $total_points += $vote_value;
    }
  }

  // Add any remaining choices that received no votes
  foreach ($node->choice as $i => $choice) {
    if (!isset($choice_votes[$i])) {
      // Didn't receive any votes
      $choice_votes[$i] = 0;
    }
  }

  // Sort descending (although there may be ties)
  arsort($choice_votes);

  // Figure out the final ranking
  $ranking = array();
  $previous_total = -1;
  $cur_result = -1;

  foreach ($choice_votes as $choice => $total) {
    if ($total != $previous_total) {
      // Didn't tie with the previous score
      $cur_result++;
    }
    $ranking[$cur_result]['choices'][] = $choice;
    $ranking[$cur_result]['rawscore'] = $total;
    $ranking[$cur_result]['viewscore'] = format_plural($total, '1 point',
      '@count points');
    $ranking[$cur_result]['percentage'] = $total_points? $total / $total_points : 0;
    $previous_total = $total;
  }

  $total_votes = count($user_votes);

  $result_obj->ranking = $ranking;
  $result_obj->total_votes = $total_votes;
  $result_obj->total_points = $total_points;
  return $result_obj;
}


/**
 * Calculate the results using instant-runoff voting.
 * 
 * @param $node
 *  The node object for the current poll.
 *
 * @return 
 *   Should return an object that include the following attributes.
 *   -results : 2d array listing the aggregate preference, including ties
 *   -rounds : 2d array listing the per-choice vote count for each round and
 *              a status message indicating who was eliminated
 *   -totalVoters : the total number of voters who participated
 */
function _advpoll_calculate_instantrunoff($node) {
   $votes = array();
  // ORDER BY value ASC lets us ensure no gaps
  $result = db_query("SELECT * FROM {votingapi_vote} v WHERE content_type='%s' AND content_id=%d ORDER BY value ASC", 'advpoll', $node->nid);
  while ($vobj = db_fetch_object($result)) {
    $votes[] = $vobj;
  }

  if (count($votes) == 0) {
    // No votes yet
    return array();
  }

  // Aggregate votes by user (uid if logged in, IP if anonymous)
  // in ascending order of value.
  $user_votes = array();

  foreach ($votes as $vote) {
    if ($vote->uid == 0) {
      // Anonymous user
      $key = $vote->hostname;
    }
    else {
      // Logged-in user
      $key = $vote->uid;
    }

    // Note: relies on ORDER BY value ASC in vote-getting SQL query.
    // Otherwise a later vote might have a lower value.
    $user_votes[$key][] = $vote->tag;
  }

  $total_votes = count($user_votes);

  // Log of 1st-place votes per choice in each round
  $round_log = array();

  // Gradually append candidates as they are eliminated; end with the winner
  $reverse_ranking = array();
  
  // If we eliminate one choice per round and have n choices, we should
  // not be able to do more than n - 1 rounds.
  $max_rounds = count($node->choice); 
  for ($round = 0; $round < $max_rounds; $round++) {

    // Initialize cur_round
    $cur_round = array();
    $total_choices = count($node->choice);

    foreach ($node->choice as $chi => $temp) {
      $cur_round[$chi] = array();
    }

    
    // Loop through each user
    foreach ($user_votes as $key => $user_vote) {
      // $user_vote[0] contains the user's first remaining preference
      $cur_round[$user_vote[0]][] = $key;
    }

    if ($round == 0) {
      // This is the first round.
      // Any choices with no first-place votes are considered eliminated.
      foreach ($cur_round as $ch => $choice_votes) {
        if (count($choice_votes) == 0) {
          unset($cur_round[$ch]);
          $reverse_ranking[0]['choices'][] = $ch;
        }
      }
    }


    // Add the current round to the matrix
    $round_log[] = $cur_round;

    // Calculate the min and max number of votes
    $min_votes = -1;
    $max_votes = 0;

    // Number of choices that have already been discarded
    $num_discarded = 0;

    // Examine the number of votes each choice received this round
    foreach ($cur_round as $ch => $choice_votes) {
      $num_votes = count($choice_votes);

      if ($num_votes > $max_votes) {
        $max_votes = $num_votes;
        // Store current winner in case it has a majority
        $cur_winner = $ch;
      }

      // This choice has already been eliminated (theoretically)
      // so don't count it as the minimum.
      if ($num_votes == 0) {
        $num_discarded++; // XXX: Probably don't need this variable any more
      }
      else if ($num_votes != 0 && ($num_votes < $min_votes || $min_votes == -1)) {
        $min_votes = $num_votes;
      }
    }

    // If one choice has a majority of remaining users it wins.
    // Note: we use count($user_votes) because some users may have incomplete
    // ballots and may have already had all of their choices eliminated.
    if ($max_votes > count($user_votes) / 2) {
    
      // Prune out the winning choice if it's still in there
      if (isset($cur_round[$cur_winner])) {
          unset($cur_round[$cur_winner]);
      }

      // Keep computing until we figure out all final rankings
      while (count($cur_round)  > 0) {
        // Loop through non-winning choices
        $current_place = array();
        $min = -1;
        foreach ($cur_round as $ch => $choice_votes) {
          // Choice has already been eliminated, just unset it
          if (count($choice_votes) == 0) {
            unset($cur_round[$ch]);
          }
          else if ($min == -1
              || count($choice_votes) < $min) {
            // New minimum
            $current_place = array($ch);
            $min = count($choice_votes);
          }
          else if (count($choice_votes) == $min) {
            // Tied for minimum
            $current_place[] = $ch;
          }
        }

        // current_place will be empty the first iteration if some
        // choices had no first-place votes and were eliminated
        // at the beginning.
        if (count($current_place) > 0) {
          $reverse_ranking[]['choices'] = $current_place;  
          // Remove all choices that had the minimum
          foreach ($current_place as $ch_key) {
            unset($cur_round[$ch_key]);
          }
        }
      }

      // Save a reversed version of the round log to help compute winnerPercent
      $revmat = array_reverse($round_log);

      // The winner finally gets added
      $reverse_ranking[]['choices'] = array($cur_winner);
      $index = count($reverse_ranking) - 1;
      $reverse_ranking[$index]['rawscore'] = round(count($revmat[0][$cur_winner]) * 100 / count($user_votes), 1);
      $reverse_ranking[$index]['viewscore'] = $reverse_ranking[$index]['rawscore'] .'%';

      $result_obj->matrix = $round_log;
      $result_obj->total_votes = $total_votes;
      $result_obj->ranking = array_reverse($reverse_ranking);
      return $result_obj;
    }
    
    // Since we're still here, no one has won, so eliminate one of the
    // choices with the lowest number of votes.

     // Find all choices with the minimum number of votes
    $min_choices = array();
    foreach ($cur_round as $ch => $choice_votes) {
      if (count($choice_votes) == $min_votes) {
        $min_choices[] = $ch;
      }
     }

    // Randomly select the choice to eliminate out of the available choices.
    // TODO: due to the randomness, this result must be cached after each vote.
    $round_loser = array_rand($min_choices);

    $reverse_ranking[]['choices'] = array($min_choices[$round_loser]);
    
    // Loop through the users who voted for the loser and redistribute
    foreach ($cur_round[$min_choices[$round_loser]] as $user_key) {
      // Remove their current first preference
      array_shift($user_votes[$user_key]);

      // Keep eliminating first preference until we run out or find an choice
      // that hasn't been eliminated.
      while ($cur_round[$user_votes[$user_key][0]] == array() && count($user_votes[$user_key]) > 0) {
        array_shift($user_votes[$user_key]);
      }

      // If they have no more preferences, remove from list for simplicity.
      if (count($user_votes[$user_key]) == 0) {
        unset($user_votes[$user_key]);
      }
    }
  }
  // Loop detected. Signal user and record.
  drupal_set_message("Could not find a solution within $max_rounds iterations.");
  $result_obj->matrix = $round_log;
  $result_obj->total_votes = $total_votes;
  return $result_obj;
}

/**
 * Implementation of the vote hook for the runoff module.
 *
 * This takes care of registering the vote in runoff nodes.
 */
function advpoll_voting_ranking_form_submit($form_id, $form_values) {
  $vote = array();
  foreach ($form_values['choice'] as $choice => $rank) {
    unset($temp);
    $temp->value = $rank;
    // A zero value indicates they didn't rank that choice
    if ($temp->value != 0) {
      $temp->value_type = 'option';
      $temp->tag = $choice;
      $vote[] = $temp;
    }
  }

  votingapi_set_vote('advpoll', $form_values['nid'], $vote);
  $node = node_load($form_values['nid']);
  _advpoll_vote_response($node, $form_values);
}

/**
 * Implementation of the vote validation hook for the runoff module.
 *
 * This checks if the submitted values are within range, if they are
 * not empty, and if they are not repeated.
 *
 * @returns boolean false on invalid forms, true otherwise.
 */
function advpoll_voting_ranking_form_validate($form_id, $form_values) {
  $node = node_load($form_values['nid']);
  $ajax = $form_values['ajax'];
  $ok = TRUE;
  
  // Check if user is eligible to vote
  if (!advpoll_eligible($node)) {
    $msg = t('You are not allowed to vote in this poll.');
    if ($ajax) {
      $errors[] = $msg;
    }
    else {
      form_set_error('choice[', $msg);
    }
    $ok = FALSE;
  }

  // Check if user has already voted
  list($voted, $cancel_vote) = _advpoll_user_voted($node->nid);
  if ($voted) {
    $msg = t('You have already voted in this poll.');
    if ($ajax) {
      $errors[] = $msg;
    }
    else {
      form_set_error('choice[', $msg);
    }
    $ok = FALSE;
  }


  // Array used to check which values are set
  $setvalues = array();
  
  $numchoices = 0;
  foreach ($node->choice as $key => $choice) {
    
    // Count the number of choices that are ranked
    if ($form_values['choice'][$key]) {
      $numchoices++;
    }
    $intvalue = intval($form_values['choice'][$key]);
    // Mark this value as seen
    $setvalues[$intvalue]++;
    // Check range
    if ($intvalue > count($node->choice) || $intvalue < 0) {
      // TODO: clean up this error message
      $msg = "Illegal rank for choice $key: $intvalue (min: 1, max: "
        . count($node->choice) .')';
      if ($ajax) {
        $errors[] = $msg;
      }
      else {
        form_set_error('choice]['. $key, $msg);
      }
      $ok = FALSE;
    }
  }
  
  // Too many choices ranked
  if ($node->maxchoices != 0 && $numchoices > $node->maxchoices) {
    $msg = t('%num choices were selected but only %max are allowed.',
        array('%num' => $numchoices, '%max' => $node->maxchoices));
    if ($ajax) {
      $errors[] = $msg;
    }
    else {
      form_set_error('choice', $msg);
    }
    $ok = false;
  }

  // Not enough choices ranked
  $minchoices = 1;
  if ($numchoices < $minchoices) {
    $msg = t('At least one choice must be selected.');
    if ($ajax) {
      $errors[] = $msg;
    }
    else {
      form_set_error('choice', $msg);
    }
    $ok = false;
  }

  // Check that multiple choices are not set to the same value
  foreach ($setvalues as $val => $count) {
    if ($val != 0 && $count > 1) {
      $msg = t('Multiple choices given the rank of %val.', array('%val' => $val));
      if ($ajax) {
        $errors[] = $msg;
      }
      else {
        form_set_error('choice', $msg);
      }
    
      $ok = false;
    }
  }
  // If the form was posted with AJAX and has errors, print the error message.
  if ($ajax && !$ok) {
    drupal_set_header('Content-Type: text/plain; charset=utf-8');
    print drupal_to_js(array('errors' => '<div class="messages error">'. implode('<br />', $errors) .'</div>'));
    exit;
  }
  // Do as usual
  else {
    return $ok;
  }
}

/**
 * Render the voting form.
 */
function theme_advpoll_voting_ranking_form($form) {
  $output = "<div class=\"poll\">\n";
  $output .= drupal_render($form);
  $output .= "</div>\n";
  return $output;
}
