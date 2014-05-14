<?php

function resource_list_page() {
  $output = '';
  $limit = 50;
  $entity = 'resource_list';
  $bookmark = resource_get_bookmark();
  $columns = resource_get_selected_columns($entity);
  $filter = empty($_SESSION['resource_filter'][$entity]) ? array() : $_SESSION['resource_filter'][$entity];
  
  $output .= drupal_get_form('resource_filter_form', $entity);
  $output .= drupal_get_form('resource_set_columns_form', $entity);
  $output .= drupal_get_form('resources_bookmarks_form', $bookmark);
  
  timer_start('resource');
  
  $query = resource_generate_query($filter, $columns, $bookmark);
  $table = resource_build_result_table($query, $limit);
  if (empty($table['rows'])) {
    $table['rows'][] = array(
      array('data' => t('No items found.'), 'colspan' => count($table['header'])),
    );
  }
  
  $timer = timer_stop('resource');
  $timer = $timer['time'];
  
  $records = $GLOBALS['pager_total_items'][0];
  
  $pager = theme('pager', array(), $limit);
  if (count($table['rows']) > 10) {
    $output .= $pager;
  }
  
  if ($records) {
    $output .= '<div class="total-records">' .
      theme('placeholder', t('Total: @records', array('@records' => $records))) .
      '</div>';
    $output .= '<div class="query-time">' .
      theme('placeholder', t('Results generated in: @time', array('@time' => $timer . ' ms'))) .
      '</div>';
    $q = $_GET;
    unset($q['q']);
    $output .= '<div class="export-icon">' . l(t('Export'), 'resource/export', array('query' => $q)) . '</div>';
  }
  
  $output .= theme('table', $table['header'], $table['rows']);
  $output .= $pager;
  
  return $output;
}

function resource_export_page() {
  $entity = 'resource_list';
  $bookmark = resource_get_bookmark();
  $columns = resource_get_selected_columns($entity);
  $filter = empty($_SESSION['resource_filter'][$entity]) ? array() : $_SESSION['resource_filter'][$entity];
  $query = resource_generate_query($filter, $columns, $bookmark);
  $table = resource_build_result_table($query);
  resource_export_csv($table, 'export.csv');
}

function resources_bookmarks_form(&$form_state, $current_bookmark) {
  $form = array();
  $bookmarks = explode(' ', "A Ă Â B C D E F G H I Î J K L M N O P Q R S Ş T Ţ U V W X Y Z");
  
  $letters = array();
  $sql = "SELECT DISTINCT LEFT(LOWER(r.lastname), 1) AS letter FROM {resource} r";
  $result = db_query($sql);
  while ($data = db_fetch_array($result)) {
    $letters[] = $data->letter;
  }
  
  $list = array();
  foreach ($bookmarks as $bookmark) {
    $letter = drupal_strtolower($bookmark);
    
    if ($bookmark == $current_bookmark) {
      $bookmark = "<strong>$bookmark</strong>";
    }
    if (in_array($bookmark, $letters)) {
      $list[] = l($bookmark, 'resources', array('html' => TRUE, 'query' => array('bookmark' => $letter)));
    }
    else {
      $list[] = $bookmark;
    }
  }
  $list[] = l('#', 'resources');

  $form['bookmarks'] = array(
    '#type' => 'fieldset',
    '#title' => t('Index'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $form['bookmarks']['letters'] = array(
    '#value' => theme('item_list', $list, NULL, array('attributes' => array('class' => 'resource_bookmarks'))),
  );

  return $form;
}

function resource_set_columns_form($form_state, $entity = '') {
  $columns = array();
  $fields = _resource_load_all_fields();
  foreach ($fields as $id => $field) {
    if (empty($field->section)) {
      $columns[$id] = check_plain($field->title);
    }
    else {
      $columns[$id] = check_plain($field->section . ' - ' . $field->title);
    }
  }
  
  $selected = resource_get_selected_columns($entity);
  
  $js = array(
    'columns' => $columns,
    'selectedColumns' => $selected,
  );
  drupal_add_js(array('resource' => $js), 'setting');
  drupal_add_js(drupal_get_path('module', 'resource') . '/js/columns.js');
  
  $form = array();
  $form['#entity'] = $entity;
  
  $form['columns'] = array(
    '#type' => 'fieldset',
    '#title' => t('Set columns'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#theme' => 'resource_columns_form',
  );
  
  $form['columns']['all_columns'] = array(
    '#type' => 'select',
    '#title' => t('All columns'),
    '#multiple' => TRUE,
    '#options' => $columns,
  );
  
  $form['columns']['move_right'] = array(
    '#type' => 'button',
    '#value' => '>',
  );
  
  $form['columns']['move_left'] = array(
    '#type' => 'button',
    '#value' => '<',
  );
  
  $form['columns']['selected'] = array(
    '#type' => 'textfield',
    '#title' => t('Selected columns'),
    '#prefix' => '<div style="display: none;">',
    '#suffix' => '</div>',
  );
  
  $form['columns']['selected_columns'] = array(
    '#type' => 'select',
    '#title' => t('Selected columns'),
    '#multiple' => TRUE,
    '#options' => array(),
  );
  
  $form['columns']['move_up'] = array(
    '#type' => 'button',
    '#value' => t('Up'),
  );
  
  $form['columns']['move_down'] = array(
    '#type' => 'button',
    '#value' => t('Down'),
  );
  
  $form['columns']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  
  $form['columns']['reset'] = array(
    '#type' => 'submit',
    '#value' => t('Reset'),
  );
  
  return $form;
}

function resource_set_columns_form_submit($form, &$form_state) {
  $entity = $form['#entity'];
  
  if ($form_state['values']['op'] == t('Save')) {
    $selected = array_filter(array_map('trim', explode(',', $form_state['values']['selected'])));
    $_SESSION['resources']['selected_columns'][$entity] = $selected;
  }
  elseif ($form_state['values']['op'] == t('Reset')) {
    if (isset($_SESSION['resources']['selected_columns'][$entity])) {
      unset($_SESSION['resources']['selected_columns'][$entity]);
    }
  }
  
  $q = $_GET;
  unset($q['q']);
  $form_state['redirect'] = array($_GET['q'], $q);
}

function theme_resource_columns_form($form) {
  $output = '';
  
  $output .= '<table class="columns-table"><tr>';
  $output .= '<td width="50%">' . drupal_render($form['all_columns']) . '</td>';
  $output .= '<td>' . drupal_render($form['move_right']) . drupal_render($form['move_left']) . '</td>';
  $output .= '<td width="50%">' . drupal_render($form['selected_columns']) . '</td>';
  $output .= '<td>' . drupal_render($form['move_up']) . drupal_render($form['move_down']) . '</td>';
  $output .= '</tr></table>';
  
  $output .= drupal_render($form);
  
  return $output;
}

function resource_filter_form($form_state, $entity) {
  $form = array(
    '#cache' => TRUE,
    '#entity' => $entity,
  );
  
  $form['filter'] = array(
    '#type' => 'fieldset',
    '#title' => t('Filter columns'),
    '#collapsible' => TRUE,
    '#collapsed' => empty($_SESSION['resource_filter'][$entity]),
    '#theme' => 'resource_filter_form_grid',
    '#entity' => $entity,
  );
  
  if (!empty($_SESSION['resource_filter'][$entity])) {
    foreach ($_SESSION['resource_filter'][$entity] as $key => $data) {
      $form['filter']['filter_' . $key] = array(
        '#type' => 'checkbox',
      );
    }
    
    $form['filter']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete selected'),
    );
    
    $form['filter']['reset'] = array(
      '#type' => 'submit',
      '#value' => t('Reset'),
    );
  }
  
  $form['filter']['add'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  
  $list = array();
  $fields = _resource_load_all_fields();
  foreach ($fields as $field) {
    if (!empty($field->searchable)) {
      if (empty($form_state['values']['attribute'])) {
        $form_state['values']['attribute'] = $field->id;
      }
      $list[$field->section][$field->id] = $field->title;
    }
  }
  $form['filter']['add']['attribute'] = array(
    '#type' => 'select',
    '#title' => t('Attribute'),
    '#options' => $list,
    '#default_value' => $form_state['values']['attribute'],
    '#ahah' => array(
      'path' => 'resource/js',
      'wrapper' => 'resource-condition',
      'method' => 'replace',
      'effect' => 'fade',
    ),
  );
  
  $form['filter']['add']['condition'] = array(
    '#prefix' => '<div id="resource-condition">',
    '#suffix' => '</div>',
  );
  
  $list = array();
  $conditions = _resource_load_all_conditions();
  foreach ($conditions as $condition) {
    if ($condition->category == $fields[$form_state['values']['attribute']]->searchable) {
      $list[$condition->id] = $condition->name;
    }
  }
  if (empty($form_state['values']['condition']) || !isset($list[$form_state['values']['condition']])) {
    reset($list);
    $form_state['values']['condition'] = key($list);
  }
  $form['filter']['add']['condition']['condition'] = array(
    '#type' => 'select',
    '#title' => t('Condition'),
    '#options' => $list,
    '#default_value' => $form_state['values']['condition'],
    '#ahah' => array(
      'path' => 'resource/js',
      'wrapper' => 'resource-condition',
      'method' => 'replace',
      'effect' => 'fade',
    ),
  );
  
  $field = $fields[$form_state['values']['attribute']];
  $condition = $conditions[$form_state['values']['condition']];
  if ($condition->num_args) {
    if ($field->type == 'date') {
      $form['filter']['add']['condition']['value1'] = array(
        '#type' => 'date',
        '#title' => t('Value 1'),
      );
    }
    elseif (!empty($field->source)) {
      if (empty($field->parsed_source)) {
        $field->parsed_source = section_build_field_options($field->source, TRUE);
      }
      
      $form['filter']['add']['condition']['value1'] = array(
        '#type' => 'checkboxes',
        '#title' => t('Value 1'),
        '#options' => $field->parsed_source,
      );
      
    }
    else {
      $form['filter']['add']['condition']['value1'] = array(
        '#type' => 'textfield',
        '#title' => t('Value 1'),
        '#size' => 20,
      );
    }
    
    if ($condition->num_args == 2) {
      $form['filter']['add']['condition']['value2'] = $form['filter']['add']['condition']['value1'];
      if (!empty($form['filter']['add']['condition']['value1']['#title'])) {
        $form['filter']['add']['condition']['value2']['#title'] = t('Value 2');
      }
    }
  }
  
  $form['filter']['add']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  
  return $form;
}

function resource_filter_form_submit($form, $form_state) {
  $entity = $form['#entity'];
  
  // The only way to prevent AHAH running form submit
  if (empty($form_state['post'])) {
    if ($form_state['values']['op'] == t('Submit')) {
      $fields = _resource_load_all_fields();
      if (!empty($fields[$form_state['values']['attribute']])) {
        $field = $fields[$form_state['values']['attribute']];
        if ($field->type == 'date') {
          if (!empty($form_state['values']['value1'])) {
            $form_state['values']['value1'] = sprintf("%d-%02d-%02d",
              $form_state['values']['value1']['year'],
              $form_state['values']['value1']['month'],
              $form_state['values']['value1']['day']);
          }
          if (!empty($form_state['values']['value2'])) {
            $form_state['values']['value2'] = sprintf("%d-%02d-%02d",
              $form_state['values']['value2']['year'],
              $form_state['values']['value2']['month'],
              $form_state['values']['value2']['day']);
          }
        }
        
        $_SESSION['resource_filter'][$entity][] = array(
          'field_id' => $form_state['values']['attribute'],
          'condition_id' => $form_state['values']['condition'],
          'value1' => empty($form_state['values']['value1']) ? '' : $form_state['values']['value1'],
          'value2' => empty($form_state['values']['value2']) ? '' : $form_state['values']['value2'],
        );
      }
    }
    elseif ($form_state['values']['op'] == t('Reset')) {
      unset($_SESSION['resource_filter'][$entity]);
    }
    elseif ($form_state['values']['op'] == t('Delete selected')) {
      foreach ($_SESSION['resource_filter'][$entity] as $key => $data) {
        if (!empty($form_state['values']['filter_' . $key])) {
          unset($_SESSION['resource_filter'][$entity][$key]);
        }
      }
    }
  }
}

function theme_resource_filter_form_grid($form) {
  $output = '';
  $entity = $form['#entity'];
  
  if (!empty($_SESSION['resource_filter'][$entity])) {
    $fields = _resource_load_all_fields();
    $conditions = _resource_load_all_conditions();
    
    $header = array(
      '',
      t('Attribute'),
      t('Condition'),
      t('Value 1'),
      t('Value 2'),
    );
    $rows = array();
    
    foreach ($_SESSION['resource_filter'][$entity] as $key => $data) {
      $field = $fields[$data['field_id']];
      $condition = $conditions[$data['condition_id']];
      
      if (!empty($field->source)) {
        if (empty($field->parsed_source)) {
          $field->parsed_source = section_build_field_options($field->source, TRUE);
        }
        
        if (!empty($data['value1'])) {
          if (is_array($data['value1'])) {
            foreach ($data['value1'] as $k => $v) {
              $data['value1'][$k] = $field->parsed_source[$v];
            }
            $data['value1'] = implode(', ', $data['value1']);
          }
          else {
            $data['value1'] = $field->parsed_source[$data['value1']];
          }
        }
        
        if (!empty($data['value2'])) {
          if (is_array($data['value2'])) {
            foreach ($data['value2'] as $k => $v) {
              $data['value2'][$k] = $field->parsed_source[$v];
            }
            $data['value2'] = implode(', ', $data['value2']);
          }
          else {
            $data['value2'] = $field->parsed_source[$data['value2']];
          }
        }
      }
      
      $rows[] = array(
        drupal_render($form['filter_' . $key]),
        $field->section . ' - ' . $field->title,
        $condition->name,
        $data['value1'],
        $data['value2'],
      );
    }
    
    $output .= theme('table', $header, $rows);
  }
  
  $output .= drupal_render($form);
  
  return $output;
}

function resource_filter_js() {
  $form_state = array(
    'storage' => NULL,
    'submitted' => FALSE,
  );
  $form_build_id = $_POST['form_build_id'];
  // Get the form from the cache.
  $form = form_get_cache($form_build_id, $form_state);
  $args = $form['#parameters'];
  $form_id = array_shift($args);
  // We will run some of the submit handlers so we need to disable redirecting.
  $form['#redirect'] = FALSE;
  // We need to process the form, prepare for that by setting a few internals
  // variables.
  $form['#post'] = $_POST;
  $form['#programmed'] = FALSE;
  $form_state['post'] = $_POST;
  // Build, validate and if possible, submit the form.
  drupal_process_form($form_id, $form, $form_state);
  // This call recreates the form relying solely on the form_state that the
  // drupal_process_form set up.
  $form = drupal_rebuild_form($form_id, $form_state, $args, $form_build_id);
  
  // Render the new output.
  $subform = $form['filter']['add']['condition'];
  unset($subform['#prefix'], $subform['#suffix']); // Prevent duplicate wrappers.
  $output = theme('status_messages') . drupal_render($subform);

  drupal_json(array('status' => TRUE, 'data' => $output));
}

function resource_get_bookmark() {
  $bookmark = '';
  if (!empty($_GET['bookmark'])) {
    if (preg_match('|^([a-z])|', drupal_strtolower($_GET['bookmark']), $match)) {
      $bookmark = $match[1];
    }
  }
  return $bookmark;
}
