<?php
// $Id$
function advpoll_binary_longname() {
  return t("Binary Poll");
}

/**
 * Implementation of the view_voting hook for the poll module.
 * 
 * This creates a list of choices to allow the user to vote on choices.
 */
function advpoll_voting_binary_form(&$node, $teaser, $page, $block) {
  if ($node->choice) {
    $list = array();
		foreach ($node->choice as $i => $choice) {
			// Don't show blank choices
			if ($choice['label']) {
     		$list[$i] = check_plain($choice['label']);
			}
    }
		$form['choice'] = array(
     	'#title' => $page ? '' : check_plain($node->title),
      '#options' => $list,
		);

		if ($node->maxchoices == 1) {
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
  );

	if (!$node->in_preview) {
  	$form['vote'] = array(
    	'#type' => 'submit',
    	'#value' => t('Vote'),
  	);
	}

  $form['#action'] = url('node/'. $node->nid);
  return $form;
}

function advpoll_view_results_binary($node, $teaser, $page, $block) {
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
	  // display results for each possible choice
		$output .= '<div class="poll">';
		foreach ($node->choice as $i => $ch) {
			if (!$votes[$i]) {
				$votes[$i] = 0;
			}
			$percentage = round(100 * $votes[$i] / $total_votes, 0);
			$output .= theme('advpoll_bar',  check_plain($ch['label']), $percentage, format_plural($votes[$i], '1 vote', '@count votes'), $block);
		}
		$output .= '</div>';
	}

	$output .= '<br />';

	return $output;
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
 * Registers the vote as a key for this node using votingapi_set_vote()
 */
function advpoll_voting_binary_form_submit($node, $form_values) {
	$vote = array();
	if ($form_values['maxchoices'] == 1) {
		// plurality voting
		$temp->value = 1;
    $temp->tag = $form_values['choice'];
	  $temp->value_type = 'option';
		$vote[] = $temp;
	}
	else {
		// approval voting
    foreach ($form_values['choice'] as $choice => $selected) {
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
  drupal_set_message(t('Your vote was registered.'));
}

/**
 * Check if the submitted key exists, just to make sure the form is not bypassed.
 *
 * @returns boolean true if the form is valid
 */
function advpoll_voting_binary_form_validate($form_id, $form_values) {
	$ok = true;
	$node = node_load($form_values['nid']);
	if ($node->maxchoices == 1) {
		// plurality voting
  	if (!($ok = array_key_exists($form_values['choice'], $node->choice))) {
    	form_set_error('choice[', 'At least one choice must be selected.');
			$ok = false;
  	}
	}
	else {
		// approval voting

		$numchoices = 0;

  	foreach ($node->choice as $i => $val) {
			// see if the box is checked
    	if ($form_values['choice'][$i]) {
				$numchoices++;
			}
  	}
	
		// too many choices ranked
		if ($node->maxchoices != 0 && $numchoices > $node->maxchoices) {
			form_set_error('choice[',
				t('%num choices were selected but only %max are allowed.',
					array('%num' => $numchoices, '%max' => $node->maxchoices)));
			$ok = false;
		}

		// not enough choices ranked
		$minchoices = 1;
		if ($numchoices < $minchoices) {
			form_set_error('choice[', t('At least one choice must be selected.'));
			$ok = false;
		}
	}
	return $ok;
}

