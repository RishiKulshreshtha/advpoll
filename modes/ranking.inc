<?php
// $Id$

function advpoll_info_ranking() {
  return array(
    'name' => 'ranking',
    'description' => t('Rank a number of choices.'),
  );
}

function advpoll_algorithms_ranking() {
  return array(
    'borda count' => t('borda count'),
    'instant runoff' => t('instant runoff'),
  );
}

function advpoll_voting_ranking_form(&$node, $teaser, $page, $block) {
  $form = array();

  static $ranking_form_count = 0; 
  $form['#id'] = 'advpoll_voting_ranking_form-' . $ranking_form_count++;
  $form['#attributes'] = array('class' => 'advpoll-vote');

  $form['ajax'] = array(
    '#type' => 'hidden',
    '#attributes' => array('class' => 'ajax'),
  );


  if ($node->choice) {
    $list = array();

    $numChoices = count($node->choice);

    // Generate the list of possible rankings
    $choices[0] = '--';
    for ($i = 1; $i <= $numChoices; $i++) {
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
        $val = t(strval($i) . 'th');
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
          '#title' => check_plain($choice['label']),
          '#options' => $choices,
        );
      }
    }
  }

  $form['in_block'] = array(
    '#type' => 'hidden',
    '#value' => $block,
  );

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
      '#attributes' => array('id' => 'edit-vote-rank-' . $ranking_vote_count++),
    );
  }

  // Multistep is set to TRUE as a hack to allow in_block to be accessed
  // when voting via ajax in a block - there may be a better way to do this.
  $form['#multistep'] = TRUE;
  $form['#action'] = url('node/'. $node->nid);
  return $form;
}

function advpoll_view_results_ranking($node, $teaser, $page, $block) {
  $results = votingapi_get_voting_results('advpoll', $node->nid);

  // If no one has voted, $results = array() and thus is empty.
  if (!empty($results)) {
    // Temporary list of choices indexes for the ranking
    $ranking_list = array();
    // Result object
    $ranking = array();
    $choices = array();
    $poll = array();
    foreach ($results as $result) {
      $tag = $result->tag;
      if ($tag == '_advpoll') {
        // Poll-wide cached value
        $poll[$result->function] = $result->value;
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
        $firstOne = true;
        $thisRank = '';

        // Loop through all choices with this ranking
        foreach ($ranking[$i]->choices as $choice) {
          $thisRank .= ($firstOne? '' : ', ')
            . check_plain($node->choice[$choice]['label']);
          $firstOne = false;
        }

        $percentage = round(100 * $ranking[$i]->percentage, 0);
        $output .= theme('advpoll_bar',  $thisRank, $percentage, $ranking[$i]->viewscore, $block);

      }
    }
    else {
      $output .= '<ol>';

      for ($i = 0; $i < count($ranking); $i++) {
        $output .= '<li> ';
        $firstOne = true;

        // Loop through all choices with this ranking
        foreach ($ranking[$i]->choices as $choice) {
          $output .= ($firstOne? '' : ', ') . check_plain($node->choice[$choice]['label']);
          $firstOne = false;
        }

        // Show the ranking's score if it exists (depends on algorithm)
        if ($ranking[$i]->viewscore) {
          $output .= ' (' . $ranking[$i]->viewscore . ')';
        }
        $output .= '</li>';
      }
    }
    $output .= '</ol>';

    // XXX: Debugging code, currently broken due to caching
    if (FALSE && user_access('administer polls') && $results->matrix) {
      $output .= '<br />Internal values, for debugging purposes:';
      // Output the table of algorithm rounds
      $output .= '<table border="1"><tr><td>Round</td>';
      foreach ($node->choice as $choice) {
        $output .= '<td>' . check_plain($choice['label']) . '</td>';
      }
      $output .= '</tr>';
      $round = 1;
      if (count($results->matrix) > 0) {
        foreach ($results->matrix as $aRound) {
          $output .= "<tr><td>$round.</td>";
          foreach ($node->choice as $key => $choicename) {
            $output .= '<td>' . count($aRound[$key]) . '</td>';
          }
          $output .= '</tr>';
          $round++;
        }
      }

      $output .= '</table>';
    }

  }
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

  // Cache total votes
  $cache['_advpoll'][0]['total_votes'] = $results->totalVotes;

  // Cache total points (if it exists)
  if (isset($results->totalPoints)) {
    $cache['_advpoll'][0]['total_points'] = $results->totalPoints;
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
  $userVotes = array();

  foreach ($votes as $vote) {
    if ($vote->uid == 0) {
      // Anonymous user
      $key = $vote->hostname;
    }
    else {
      // Logged-in user
      $key = $vote->uid;
    }

    $userVotes[$key][$vote->value] = $vote->tag;
  }

  $choiceVotes = array();

  $totalChoices = count($node->choice);
  $totalPoints = 0;

  // Loop through each user's vote
  foreach ($userVotes as $uid => $userVote) {
    foreach ($userVote as $ranking => $choice) {
      // Negative values are possible if choices were removed after vote
      $voteValue = max($totalChoices - $ranking, 0);
      $choiceVotes[$choice] +=   $voteValue;
      $totalPoints += $voteValue;
    }
  }

  // Add any remaining choices that received no votes
  foreach ($node->choice as $i => $choice) {
    if (!isset($choiceVotes[$i])) {
      // Didn't receive any votes
      $choiceVotes[$i] = 0;
    }
  }

  // Sort descending (although there may be ties)
  arsort($choiceVotes);

  // Figure out the final ranking
  $ranking = array();
  $previousTotal = -1;
  $curResult = -1;

  foreach ($choiceVotes as $choice => $total) {
    if ($total != $previousTotal) {
      // Didn't tie with the previous score
      $curResult++;
    }
    $ranking[$curResult]['choices'][] = $choice;
    $ranking[$curResult]['rawscore'] = $total;
    $ranking[$curResult]['viewscore'] = format_plural($total, '1 point',
      '@count points');
    $ranking[$curResult]['percentage'] = $totalPoints? $total / $totalPoints : 0;
    $previousTotal = $total;
  }

  $totalVotes = count($userVotes);

  $resultObj->ranking = $ranking;
  $resultObj->totalVotes = $totalVotes;
  $resultObj->totalPoints = $totalPoints;
  return $resultObj;
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
  $userVotes = array();

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
    $userVotes[$key][] = $vote->tag;
  }

  $totalVotes = count($userVotes);

  // Log of 1st-place votes per choice in each round
  $roundLog = array();

  // Gradually append candidates as they are eliminated; end with the winner
  $reverseRanking = array();
  
  // If we eliminate one choice per round and have n choices, we should
  // not be able to do more than n - 1 rounds.
  $maxRounds = count($node->choice); 
  for ($round = 0; $round < $maxRounds; $round++) {

    // Initialize curRound
    $curRound = array();
    $totalChoices = count($node->choice);

    foreach ($node->choice as $chi => $temp) {
      $curRound[$chi] = array();
    }

    
    // Loop through each user
    foreach ($userVotes as $key => $userVote) {
      // $userVote[0] contains the user's first remaining preference
      $curRound[$userVote[0]][] = $key;
    }

    if ($round == 0) {
      // This is the first round.
      // Any choices with no first-place votes are considered eliminated.
      foreach ($curRound as $ch => $choiceVotes) {
        if (count($choiceVotes) == 0) {
          unset($curRound[$ch]);
          $reverseRanking[0]['choices'][] = $ch;
        }
      }
    }


    // Add the current round to the matrix
    $roundLog[] = $curRound;

    // Calculate the min and max number of votes
    $minVotes = -1;
    $maxVotes = 0;

    // Number of choices that have already been discarded
    $num_discarded = 0;

    // Examine the number of votes each choice received this round
    foreach ($curRound as $ch => $choiceVotes) {
      $numVotes = count($choiceVotes);

      if ($numVotes > $maxVotes) {
        $maxVotes = $numVotes;
        // Store current winner in case it has a majority
        $curWinner = $ch;
      }

      // This choice has already been eliminated (theoretically)
      // so don't count it as the minimum.
      if ($numVotes == 0) {
        $num_discarded++; // XXX: Probably don't need this variable any more
      }
      else if ($numVotes != 0 && ($numVotes < $minVotes || $minVotes == -1)) {
        $minVotes = $numVotes;
      }
    }

    // If one choice has a majority of remaining users it wins.
    // Note: we use count($userVotes) because some users may have incomplete
    // ballots and may have already had all of their choices eliminated.
    if ($maxVotes > count($userVotes) / 2) {
    
      // Prune out the winning choice if it's still in there
      if (isset($curRound[$curWinner])) {
          unset($curRound[$curWinner]);
      }

      // Keep computing until we figure out all final rankings
      while (count($curRound)  > 0) {
        // Loop through non-winning choices
        $currentPlace = array();
        $min = -1;
        foreach ($curRound as $ch => $choiceVotes) {
          // Choice has already been eliminated, just unset it
          if (count($choiceVotes) == 0) {
            unset($curRound[$ch]);
          }
          else if ($min == -1
              || count($choiceVotes) < $min) {
            // New minimum
            $currentPlace = array($ch);
            $min = count($choiceVotes);
          }
          else if (count($choiceVotes) == $min) {
            // Tied for minimum
            $currentPlace[] = $ch;
          }
        }

        // currentPlace will be empty the first iteration if some
        // choices had no first-place votes and were eliminated
        // at the beginning.
        if (count($currentPlace) > 0) {
          $reverseRanking[]['choices'] = $currentPlace;  
          // Remove all choices that had the minimum
          foreach ($currentPlace as $chKey) {
            unset($curRound[$chKey]);
          }
        }
      }

      // Save a reversed version of the round log to help compute winnerPercent
      $revmat = array_reverse($roundLog);

      // The winner finally gets added
      $reverseRanking[]['choices'] = array($curWinner);
      $index = count($reverseRanking) - 1;
      $reverseRanking[$index]['rawscore'] = round(count($revmat[0][$curWinner]) * 100 / count($userVotes), 1);
      $reverseRanking[$index]['viewscore'] = $reverseRanking[$index]['rawscore'] . '%';

      $resultObj->matrix = $roundLog;
      $resultObj->totalVotes = $totalVotes;
      $resultObj->ranking = array_reverse($reverseRanking);
      return $resultObj;
    }
    
    // Since we're still here, no one has won, so eliminate one of the
    // choices with the lowest number of votes.

     // Find all choices with the minimum number of votes
    $minChoices = array();
    foreach ($curRound as $ch => $choiceVotes) {
      if (count($choiceVotes) == $minVotes) {
        $minChoices[] = $ch;
      }
     }

    // Randomly select the choice to eliminate out of the available choices.
    // TODO: due to the randomness, this result must be cached after each vote.
    $roundLoser = array_rand($minChoices);

    $reverseRanking[]['choices'] = array($minChoices[$roundLoser]);
    
    // Loop through the users who voted for the loser and redistribute
    foreach ($curRound[$minChoices[$roundLoser]] as $userKey) {
      // Remove their current first preference
      array_shift($userVotes[$userKey]);

      // Keep eliminating first preference until we run out or find an choice
      // that hasn't been eliminated.
      while ($curRound[$userVotes[$userKey][0]] == array()
        && count($userVotes[$userKey]) > 0)
      {
        array_shift($userVotes[$userKey]);
      }

      // If they have no more preferences, remove from list for simplicity.
      if (count($userVotes[$userKey]) == 0) {
        unset($userVotes[$userKey]);
      }
    }
  }
  // Loop detected. Signal user and record.
  drupal_set_message("Could not find a solution within $maxRounds iterations.");
  $resultObj->matrix = $roundLog;
  $resultObj->totalVotes = $totalVotes;
  return $resultObj;
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

  // Check if user has already voted
  list($voted, $cancel_vote) = _advpoll_user_voted($node);
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
        . count($node->choice) . ')';
      if ($ajax) {
        $errors[] = $msg;
      }
      else {
        form_set_error('choice][' . $key, $msg);
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
  $output = '<div class="poll">'. "\n";
  $output .= drupal_render($form);
  $output .= "</div>\n";
  return $output;
}
