<?php
// $Id$

function advpoll_ranking_longname() {
  return t("Ranking");
}

function advpoll_view_voting_ranking(&$node, $teaser, $page, $block) {

  $weight = 0;
	$form = array();

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
			'#type' => 'checkboxes', //XXX: Workaround for FormAPI bug in PHP 4
      '#tree' => TRUE,
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

  $form['nid'] = array(
    '#type' => 'hidden',
    '#value' => $node->nid,
    '#weight' => $weight++,
  );

	if (!$node->in_preview) {
  	$form['vote'] = array(
    	'#type' => 'submit',
    	'#value' => t('Vote'),
    	'#weight' => $weight++,
  	);
	}

  $form['#action'] = url('node/'. $node->nid);
  return drupal_get_form('advpoll_view_voting', $form);
}

function advpoll_view_results_ranking($node, $teaser, $page, $block) {
	$results = votingapi_get_voting_results('advpoll', $node->nid);
  $round_table = '';

	// If no one has voted, $results = array() and thus is empty
	if (!empty($results)) {
		// temporary list of choices indexes for the ranking
		$ranking_list = array();
		// result object
		$ranking = array();
		$choices = array();
		$poll = array();
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
			$output .= '<div class="poll">';
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
			$output .= '</div>';
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
					$output .= ' (' . $ranking[$i]->viewscore . '%)';
				}
				$output .= '</li>';
			}
		}
		$output .= '</ol>';

		if (!$block && user_access('inspect all votes') && isset($rounds)) {
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
        $round_table = theme('table', $header, $rows, array(), '') . '<br />';
			}
		}
    $output .= $round_table;
    return $output;
	}
}

/**
 * Calculate the results for a ranking poll based on the algorithm
 * 
 * @param $node
 *	The node object for the current poll
 *
 * @return 
 * 	Should return an object that include the following attributes
 *   -results : 2d array listing the aggregate preference, including ties
 *   -rounds : 2d array listing the per-choice vote count for each round and
 *							a status message indicating who was eliminated
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
      $key = '_rounds_' . $i;
      $cache[$key] = array();
      foreach ($round as $choice => $votes) {
        $cache[$key][0][$choice] = count($votes);
      }
    }
  }

	// Cache total votes
	$cache['_advpoll'][0]['total_voters'] = $results->totalVotes;

	// Cache total points (if it exists)
	if (isset($results->totalPoints)) {
		$cache['_advpoll'][0]['total_points'] = $results->totalPoints;
	}
}

/**
 * Calculate the results using borda count
 * 
 * @param $node
 *	The node object for the current poll
 *
 * @return 
 * 	Should return an object that include the following attributes
 *   -results : 2d array listing the aggregate preference, including ties
 *   -rounds : 2d array listing the per-choice vote count for each round and
 *							a status message indicating who was eliminated
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
		// no votes yet
		return array();
	}

	// aggregate votes by user (uid if logged in, IP if anonymous)
	// in ascending order of value
	$userVotes = array();

  foreach ($votes as $vote) {
		if ($vote->uid == 0) {
			// anonymous user
			$key = $vote->hostname;
		}
		else {
			// logged-in user
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
			$choiceVotes[$choice] += 	$voteValue;
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

	// sort descending (although there may be ties)
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
			'%count points');
		$ranking[$curResult]['percentage'] = $total / $totalPoints;
		$previousTotal = $total;
	}

	$totalVotes = count($userVotes);

	$resultObj->ranking = $ranking;
	$resultObj->totalVotes = $totalVotes;
	$resultObj->totalPoints = $totalPoints;
  return $resultObj;
}


/**
 * Calculate the results using instant-runoff voting
 * 
 * @param $node
 *	The node object for the current poll
 *
 * @return 
 * 	Should return an object that include the following attributes
 *   -results : 2d array listing the aggregate preference, including ties
 *   -rounds : 2d array listing the per-choice vote count for each round and
 *							a status message indicating who was eliminated
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
		// no votes yet
		return array();
	}

	// aggregate votes by user (uid if logged in, IP if anonymous)
	// in ascending order of value
	$userVotes = array();

  foreach ($votes as $vote) {
		if ($vote->uid == 0) {
			// anonymous user
			$key = $vote->hostname;
		}
		else {
			// logged-in user
			$key = $vote->uid;
		}

		// Note: relies on ORDER BY value ASC in vote-getting SQL query
		// Otherwise a later vote might have a lower value
		$userVotes[$key][] = $vote->tag;
	}

	$totalVotes = count($userVotes);

	// log of 1st-place votes per choice in each round
	$roundLog = array();

	// Gradually append candidates as they are eliminated; end with the winner
	$reverseRanking = array();
	
	// If we eliminate one choice per round and have n choices, we should
	// not be able to do more than n - 1 rounds
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
			// This is the first round
			// Any choices with no first-place votes are considered eliminated
  		foreach ($curRound as $ch => $choiceVotes) {
				if (count($choiceVotes) == 0) {
					unset($curRound[$ch]);
					$reverseRanking[0]['choices'][] = $ch;
				}
			}
		}


    // Add the current round to the matrix
    $roundLog[] = $curRound;

    //Calculate the min and max number of votes
    $minVotes = -1;
		$maxVotes = 0;

		// Number of choices that have already been discarded
    $num_discarded = 0;

		// examine the number of votes each choice received this round
    foreach ($curRound as $ch => $choiceVotes) {
			$numVotes = count($choiceVotes);

      if ($numVotes > $maxVotes) {
        $maxVotes = $numVotes;
			  $curWinner = $ch; // store current winner in case it has a majority
      }

			// This choice has already been eliminated (theoretically)
			// so don't count it as the minimum
			if ($numVotes == 0) {
				$num_discarded++; // probably don't need this variable any more
			}
			else if ($numVotes != 0 && ($numVotes < $minVotes || $minVotes == -1)) {
        $minVotes = $numVotes;
      }
    }

		// If one choice has a majority of remaining users it wins
		// Note: we use count($userVotes) because some users may have incomplete
		// ballots and may have already had all of their choices eliminated
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
						//drupal_set_message('New minimum: '. $ch .'('
							//. count($choiceVotes) . ')');
					}
					else if (count($choiceVotes) == $min) {
						// Tied for minimum
						$currentPlace[] = $ch;
					}
				}

				// currentPlace will be empty the first iteration if some
				// choices had no first-place votes and were eliminated
				// at the beginning
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

		// Randomly select the choice to eliminate out of the available choices
		// TODO: due to the randomness, this result must be cached after each vote
		$roundLoser = array_rand($minChoices);

		$reverseRanking[]['choices'] = array($minChoices[$roundLoser]);
		
		// Loop through the users who voted for the loser and redistribute
		foreach ($curRound[$minChoices[$roundLoser]] as $userKey) {
			// Remove their current first preference
			array_shift($userVotes[$userKey]);

			// Keep eliminating first preference until we run out or find an choice
			// that hasn't been eliminated
			while ($curRound[$userVotes[$userKey][0]] == array()
				&& count($userVotes[$userKey]) > 0)
			{
				array_shift($userVotes[$userKey]);
			}

			// If they have no more preferences, remove from list for simplicity
			if (count($userVotes[$userKey]) == 0) {
				unset($userVotes[$userKey]);
			}
		}
	}
  // loop detected. signal user and record.
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
function advpoll_vote_ranking($node) {
  foreach ($node->choice as $key => $choice) {
    $vote->value = $_POST['edit']['choice'][$key];

		// A zero value indicates they didn't rank that choice
		if ($vote->value != 0) {
    	$vote->value_type = VOTINGAPI_VALUE_TYPE_KEY;
    	$vote->tag = $key;
    	votingapi_set_vote('advpoll', $node->nid, $vote);
		}
  }
}

/**
 * implementation of the vote validation hook for the runoff module.
 *
 * This checks if the submitted values are within range, if they are
 * not empty, and if they are not repeated.
 *
 * @returns boolean false on invalid forms, true otherwise.
 */
function advpoll_vote_validate_ranking($node) {
  $ok = TRUE;
  // array used to check which values are set
  $setvalues = array();
	
	$numchoices = 0;
  foreach ($node->choice as $key => $choice) {
		
		// count the number of choices that are ranked
    if (!empty($_POST['edit']['choice'][$key])) {
			$numchoices++;
		}
    $intvalue = intval($_POST['edit']['choice'][$key]);
    // mark this value as seen
    $setvalues[$intvalue]++;
    // check range
    if ($intvalue > count($node->choice) || $intvalue < 0) {
      form_set_error('choice]['.$key, "illegal rank for choice $key: $intvalue (min: 1, max: " . count($node->choice) . ")");
      $ok = FALSE;
    }

  }
	
	// too many choices ranked
	if ($node->maxchoices != 0 && $numchoices > $node->maxchoices) {
		form_set_error('choice',
			t('%num choices were selected but only %max are allowed.',
				array('%num' => $numchoices, '%max' => $node->maxchoices)));
		$ok = false;
	}

	// not enough choices ranked
	$minchoices = 1;
	if ($numchoices < $minchoices) {
		form_set_error('choice', t('At least one choice must be selected.'));
		$ok = false;
	}

	// Check that multiple choices are not set to the same value
	foreach ($setvalues as $val => $count) {
		if ($val != 0 && $count > 1) {
			form_set_error('choice',
				t('Multiple choices given the rank of %val.', array('%val' => $val)));
			$ok = false;
		}
	}

  return $ok;
}
