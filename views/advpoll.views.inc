<?php

/**
 * @file
 * Views integration for Advanced Poll module.
 */

/**
 * Implementation of hook_views_data()
 */
function advpoll_views_data() {
    // Basic table information.
    $data['advpoll']['table']['group'] = t('Advanced Poll');

    // Join to 'node' as a base table.
    $data['advpoll']['table']['join'] = array(
        'node' => array(
            'left_field' => 'nid',
            'field' => 'nid',
        ),
    );

    // ----------------------------------------------------------------
    // Fields

    $data['advpoll']['active'] = array(
        'title' => t('Active'),
        'help' => t('Whether the poll is open for voting.'),
        'field' => array(
            'handler' => 'views_handler_field_boolean',
            'click sortable' => TRUE,
        ),
        'filter' => array(
            'handler' => 'views_handler_filter_boolean_operator',
            'label' => t('Active'),
            'type' => 'yes-no',
        ),
        'sort' => array(
            'handler' => 'views_handler_sort',
        ),
    );
    //MW added to allow display/sorting in views
    $data['advpoll']['create_view_block'] = array(
        'title' => t('Create View Block'),
        'help' => t('Whether or not to automatically build a view block.'),
        'field' => array(
            'handler' => 'views_handler_field_boolean',
            'click sortable' => TRUE,
        ),
        'filter' => array(
            'handler' => 'views_handler_filter_boolean_operator',
            'label' => t('Create View Block'),
            'type' => 'yes-no',
        ),
        'sort' => array(
            'handler' => 'views_handler_sort',
        ),
    );    

    $data['advpoll']['start_date'] = array(
        'title' => t('Start date'),
        'help' => t('The date at which voting begins for the poll.'),
        'field' => array(
            'handler' => 'views_handler_field_date',
            'click sortable' => TRUE,
        ),
        'sort' => array(
            'handler' => 'views_handler_sort_date',
        ),
        'filter' => array(
            'handler' => 'views_handler_filter_date',
        ),
    );

    $data['advpoll']['end_date'] = array(
        'title' => t('End date'),
        'help' => t('The date at which voting ends for the poll.'),
        'field' => array(
            'handler' => 'views_handler_field_date',
            'click sortable' => TRUE,
        ),
        'sort' => array(
            'handler' => 'views_handler_sort_date',
        ),
        'filter' => array(
            'handler' => 'views_handler_filter_date',
        ),
    );

    $data['advpoll']['question'] = array(
        'title' => t('Question'),
        'help' => t('Optional text to display as the subject of the poll.'),
        'field' => array(
            'handler' => 'views_handler_field',
            'click sortable' => TRUE,
        ),
        'sort' => array(
            'handler' => 'views_handler_sort',
        ),
        'filter' => array(
            'handler' => 'views_handler_filter_string',
        ),
        'argument' => array(
            'handler' => 'views_handler_argument_string',
        ),
    );

    return $data;
}

function advpoll_views_default_views() {
    // MW builds the base view and calls _advpoll_generate_blocks function to 
    // append with custom blocks
    $views = array();

    // Exported view: advpoll_views
    $view = new view;
    $view->name = 'advpoll_views';
    $view->description = '';
    $view->tag = '';
    $view->base_table = 'node';
    $view->core = 6;
    $view->api_version = '2';
    $view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */
    $handler = $view->new_display('default', 'Defaults', 'default');
    $handler->override_option('filters', array(
        'status' => array(
            'operator' => '=',
            'value' => '1',
            'group' => '0',
            'exposed' => FALSE,
            'expose' => array(
                'operator' => FALSE,
                'label' => '',
            ),
            'id' => 'status',
            'table' => 'node',
            'field' => 'status',
            'relationship' => 'none',
        ),
        'type' => array(
            'operator' => 'in',
                'value' => array(
                    'advpoll_binary' => 'advpoll_binary',
                    'advpoll_ranking' => 'advpoll_ranking',
                ),
            'group' => '0',
            'exposed' => FALSE,
            'expose' => array(
                'operator' => FALSE,
                'label' => '',
            ),
            'id' => 'type',
            'table' => 'node',
            'field' => 'type',
            'relationship' => 'none',
        ),
        'end_date' => array(
            'operator' => '>',
            'value' => array(
                'type' => 'offset',
                'value' => 'now',
                'min' => '',
                'max' => '',
            ),
            'group' => '0',
            'exposed' => FALSE,
            'expose' => array(
                'operator' => FALSE,
                'label' => '',
            ),
            'id' => 'end_date',
            'table' => 'advpoll',
            'field' => 'end_date',
            'relationship' => 'none',
        ),
        'start_date' => array(
            'operator' => '<',
            'value' => array(
                'type' => 'offset',
                'value' => 'now',
                'min' => '',
                'max' => '',
            ),
            'group' => '0',
            'exposed' => FALSE,
            'expose' => array(
                'operator' => FALSE,
                'label' => '',
            ),
            'id' => 'start_date',
            'table' => 'advpoll',
            'field' => 'start_date',
            'relationship' => 'none',
        ),
    ));
    $handler->override_option('access', array(
        'type' => 'none',
    ));
    $handler->override_option('cache', array(
        'type' => 'none',
    ));
    $handler->override_option('items_per_page', 1);
    $handler->override_option('row_plugin', 'node');
    $handler->override_option('row_options', array(
        'relationship' => 'none',
        'build_mode' => 'full',
        'links' => 0,
        'comments' => 0,
    ));

    _advpoll_generate_blocks($view);
    /*

     */
    $views[$view->name] = $view;

    return $views;
}

function _advpoll_generate_blocks($view) {
    // MW generates custom view blocks based on selection criteria
    $result = db_query('SELECT * FROM {node} n INNER JOIN {advpoll} p ON p.nid = n.nid WHERE n.status = 1 
                        AND p.create_view_block = 1 AND p.start_date < %d 
                        AND p.end_date > %d', time(), time());

    // MW note that the id of a given block is unique since it's based on the displayed node's
    // nid.  This is done so that each time the view is refreshed, the blocks can be regenerated without
    // regard for order - this ensures that a specific block placed on a page will remain there even if
    // the view is rebuilt (as it is each time a poll is edited or created).
    while ($node = db_fetch_object($result)) {
        $handler = $view->new_display('block', 'Poll Block - ' . $node->title, 'block_' . $node->nid);
        $handler->override_option('filters', array(
            'status' => array(
                'operator' => '=',
                'value' => '1',
                'group' => '0',
                'exposed' => FALSE,
                'expose' => array(
                    'operator' => FALSE,
                    'label' => '',
                ),
                'id' => 'status',
                'table' => 'node',
                'field' => 'status',
                'relationship' => 'none',
            ),
            'type' => array(
                'operator' => 'in',
                'value' => array(
                    'advpoll_binary' => 'advpoll_binary',
                    'advpoll_ranking' => 'advpoll_ranking',
                ),
                'group' => '0',
                'exposed' => FALSE,
                'expose' => array(
                    'operator' => FALSE,
                    'label' => '',
                ),
                'id' => 'type',
                'table' => 'node',
                'field' => 'type',
                'relationship' => 'none',
            ),
            'create_view_block' => array(
                'operator' => '=',
                'value' => '1',
                'group' => '0',
                'exposed' => FALSE,
                'expose' => array(
                    'operator' => FALSE,
                    'label' => '',
                ),
                'id' => 'create_view_block',
                'table' => 'advpoll',
                'field' => 'create_view_block',
                'relationship' => 'none',
            ),
            'end_date' => array(
                'operator' => '>',
                'value' => array(
                    'type' => 'offset',
                    'value' => 'now',
                    'min' => '',
                    'max' => '',
                ),
                'group' => '0',
                'exposed' => FALSE,
                'expose' => array(
                    'operator' => FALSE,
                    'label' => '',
                ),
                'id' => 'end_date',
                'table' => 'advpoll',
                'field' => 'end_date',
                'relationship' => 'none',
            ),
            'start_date' => array(
                'operator' => '<',
                'value' => array(
                    'type' => 'offset',
                    'value' => 'now',
                    'min' => '',
                    'max' => '',
                ),
                'group' => '0',
                'exposed' => FALSE,
                'expose' => array(
                    'operator' => FALSE,
                    'label' => '',
                ),
                'id' => 'start_date',
                'table' => 'advpoll',
                'field' => 'start_date',
                'relationship' => 'none',
            ),
            'nid' => array(
                'operator' => '=',
                'value' => array(
                    'value' => $node->nid,
                    'min' => '',
                    'max' => '',
                ),
                'group' => '0',
                'exposed' => FALSE,
                'expose' => array(
                    'operator' => FALSE,
                    'label' => '',
                ),
                'id' => 'nid',
                'table' => 'node',
                'field' => 'nid',
                'override' => array(
                    'button' => 'Use default',
                ),
                'relationship' => 'none',
            ),
        ));
        $handler->override_option('block_description', '');
        $handler->override_option('block_caching', -1);
    }
}