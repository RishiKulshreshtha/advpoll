<?php

function advpoll_views_tables() {

  $tables['votingapi_advpoll'] = array(
    "name" => "votingapi_vote", 
    "provider" => "votingapi",
    "join" => array(
      "left" => array(
        "table" => "node",
        "field" => "nid"
      ), 
      "right" => array(
        "field" => "content_id"
      ),
      "extra" => array(
        'content_type' => 'advpoll',
        'value_type' => 'option',
      ),
    ),
    "fields" => array(
      "value" => array(
        'name' => t('Advanced Poll: value'),
        'handler' => votingapi_get_formatters(array('value_type' => 'option')),
        'value_type' => 'option',
        'sortable' => TRUE,
      ),
      "timestamp" => array(
        'name' => t('Advanced Poll: timestamp'),
        'sortable' => TRUE,
        'handler' => 
          array(
            "views_handler_field_date_small" => t('As Short Date'), 
            "views_handler_field_date" => t('As Medium Date'), 
            "views_handler_field_date_large" => t('As Long Date'), 
            "views_handler_field_since" =>  t('As Time Ago')
          ),
        'sortable' => TRUE,
        'help' => "Display the time the vote was cast.",
      ),
      "uid" => array(
        'name' => t('Advanced Poll: user'),
        'handler' => 'votingapi_views_handler_field_username', 
        'sortable' => TRUE, 
      ),
    ),  
    "sorts" => array(
      "value" => array('name' => t('Advanced Poll: vote')),
      "uid" => array('name' => t('Advanced Poll: voter')),
      "timestamp" => array('name' => t('Advanced Poll: timestamp')),
    ),
    'filters' => array(
      'value' => array(
        'name' => t('Advanced Poll: value'),
        'operator' => 'views_handler_operator_gtlt',
        'handler' => 'votingapi_handler_filter_value',
        'value-type' => 'string',
        'help' => t('Filter nodes by values of the individual votes users cast for them.'),
      ),
      'timestamp' => array(
        'name' => t('Advanced Poll: timestamp'),
        'operator' => 'views_handler_operator_gtlt',
        'value' => views_handler_filter_date_value_form(),
        'handler' => 'views_handler_filter_timestamp',
        'option' => 'string',
        'help' => t('Filter nodes by the date they were voted on.')
          .' '. views_t_strings('filter date'),
      ),
      'uid' => array(
        'name' => t('Advanced Poll: user'),
        'operator' => array('IS NOT NULL' => t('Has been voted on by'), 'IS NULL' => t('Has not been voted on by')),
        'list' => array('***CURRENT_USER***' => t('Currently Logged In User'), '***ANY_USER***' => t('Any user')),
        'list-type' => 'select',
        'handler' => 'votingapi_handler_filter_uid_voted',
        'help' => t('Filter nodes by whether the currently logged in user has voted.'),
        'content_type' => 'advpoll',
        'value_type' => 'option',
      )
    ),
  );

  $tables['advpoll'] = array(
    'name' => 'advpoll',
    'provider' => 'advpoll',
    'join' => array(
      'left' => array(
        'table' => 'node',
        'field' => 'nid',
      ),
      'right' => array(
        'field' => 'nid',
      ),
    ),
    'fields' => array(
      'active' => array(
        'name' => t('Advanced Poll: Status'),
        'help' => 'If a poll is open for votes or closed.',
        'sortable' => TRUE,
        'handler' => 'advpoll_views_active_field_handler',
      ),
      'mode' => array(
        'name' => t('Advanced Poll: Mode'),
        'sortable' => TRUE,
        'help' => 'Polls can be either ranking or yes/no style.',
      ),
      'algorithm' => array(
        'name' => t('Advanced Poll: Algorithm'),
        'sortable' => TRUE,
        'help' => 'The voting system that calculates who wins a poll.',
      ),
    ),
    'filters' => array(
      'active' => array(
        'name' => t('Advanced Poll: Status'),
        'help' => 'If a poll is open for votes or closed.',
        'operator' => array('=' => t('Is'), '!=' => t('Is Not')),
        'value' => array(
          '#type' => 'select',
          '#options' => array(t('Closed'), t('Open'))
        )
      ),
    ),
  );

  $tables['votingapi_advpoll_current_user'] = array(
    "name" => "votingapi_vote", 
    "provider" => "votingapi",
    "join" => array(
      "left" => array(
        "table" => "node",
        "field" => "nid"
      ), 
      "right" => array(
        "field" => "content_id"
      ),
      "extra" => array(
        'content_type' => 'advpoll',
        'value_type' => 'option',
        'uid' => '***CURRENT_USER***'
      ),
    ),
    "fields" => array(
      "value" => array(
        'name' => t('Advanced Poll: value (current user only)'),
        'field' => 'value',
        'handler' => votingapi_get_formatters(array('value_type' => 'option')),
        'value_type' => 'option',
        'sortable' => TRUE,
      ),

      "timestamp" => array(
        'name' => t('Advanced Poll: timestamp (current user only)'),
        'sortable' => TRUE,
        'handler' => 
          array(
            "views_handler_field_date_small" => t('As Short Date'), 
            "views_handler_field_date" => t('As Medium Date'), 
            "views_handler_field_date_large" => t('As Long Date'), 
            "views_handler_field_since" =>  t('As Time Ago')
          ),
        'sortable' => TRUE,
        'help' => "Display the time the vote was cast.",
      ),
    ),  
    "sorts" => array(
      "value" => array('name' => t("Advanced Poll: !type vote", array('!type' => 'option'))),
      "timestamp" => array('name' => t("Advanced Poll: !type timestamp", array('!type' => 'option'))),
    ),
    'filters' => array(
      'value' => array(
        'name' => t('Advanced Poll: value (current user only)'),
        'operator' => 'views_handler_operator_gtlt',
        'handler' => 'votingapi_handler_filter_value',
        'value-type' => 'string',
        'help' => t("Filter nodes by values of the current user's vote for them."),
      ),
      'timestamp' => array(
        'name' => t('Advanced Poll: timestamp (current user only)'),
        'operator' => 'views_handler_operator_gtlt',
        'value' => views_handler_filter_date_value_form(),
        'handler' => 'views_handler_filter_timestamp',
        'option' => 'string',
        'help' => t('Filter nodes by the date they were voted on by the current user.')
          .' '. views_t_strings('filter date'),
      ),
    ),
  );

  return $tables;
}

/**
 * Views handler to format the "active" status field.
 */
function advpoll_views_active_field_handler($fieldinfo, $fielddata, $value, $data) {
  return $value? t('Open') : t('Closed');
}
