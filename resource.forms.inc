<?php

function resource_edit_form($form_state, $section_machine_name, $resource_id = 0, $id = 0) {
  $section = section_load($section_machine_name);
  
  if ($resource_id) {
    $resource = resource_load($resource_id);
    $resource_name = trim($resource->firstname . ' ' . $resource->lastname);
    if ($section_machine_name == 'resource') {
      drupal_set_title(t('Edit @name', array('@name' => $resource_name)));
    }
    else {
      global $language;
      $attr = array(
        '@name' => $resource_name,
        '@section' => $section->details[$language->language]['title'],
      );
      drupal_set_title(t('Edit @section for @name', $attr));
    }
    
    $breadcrumb = array(
      l(t('Home'), '<front>'),
      l(t('Resources'), 'resource'),
      l($resource_name, 'resource/' . $resource_id . '/' . $section_machine_name),
    );
    drupal_set_breadcrumb($breadcrumb);
  }
  else {
    drupal_set_title(t('Add resource'));
  }
  
  $form = section_build_form($form_state, $section, $resource_id, $id);
  
  if (!empty($form['#entity']->id)) {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
    );
  }
  
  $url = 'resource';
  if (!empty($resource_id)) {
    $url .= '/' . $resource_id;
    if ($section_machine_name != 'resource') {
      $url .= '/' . $section_machine_name;
    }
  }
  $form['actions']['cancel'] = array(
    '#value' => l(t('Cancel'), $url),
  );
  
  return $form;
}

function resource_edit_form_validate($form, &$form_state) {
  if ($form_state['values']['op'] != t('Delete')) {
    return section_build_form_validate($form, &$form_state);
  }
}

function resource_edit_form_submit($form, &$form_state) {
  if ($form_state['values']['op'] == t('Delete')) {
    if ($form['#section']->machine_name == 'resource') {
      $form_state['redirect'] = 'resource/' . $form['#entity_id'] . '/delete';
    }
    else {
      $form_state['redirect'] = 'resource/' . $form['#entity_id'] .
        '/' . $form['#section']->machine_name .
        '/' . $form['#record_id'] . '/delete';
    }
    return;
  }
  
  if ($form['#section']->machine_name == 'resource') {
    if (empty($form['#entity']->author_uid)) {
      global $user;
      $form['#entity']->author_uid = $user->uid;
    }
    $form['#entity']->modified = time();
    if (empty($form['#entity']->created)) {
      $form['#entity']->created = $form['#entity']->modified;
    }
  }
  
  section_build_form_submit($form, $form_state);
  
  $url = 'resource';
  if (!empty($form['#entity_id'])) {
    $url .= '/' . $form['#entity_id'];
    if ($form['#section']->machine_name != 'resource') {
      $url .= '/' . $form['#section']->machine_name;
    }
  }
  $form_state['redirect'] = array($url);
}

function resource_delete_form($form_state, $section_machine_name, $resource_id, $id = 0) {
  $section = section_load($section_machine_name);
  if (empty($section)) {
    drupal_not_found();
    exit();
  }
  
  $entity = section_entity_load($section, $resource_id, $id);
  if (empty($entity) || is_array($entity)) {
    drupal_not_found();
    exit();
  }
  
  $resource = resource_load($resource_id);
  $resource_name = trim($resource->firstname . ' ' . $resource->lastname);
  $breadcrumb = array(
    l(t('Home'), '<front>'),
    l(t('Resources'), 'resource'),
    l($resource_name, 'resource/' . $resource_id . '/' . $section_machine_name),
  );
  drupal_set_breadcrumb($breadcrumb);
  
  
  if ($section->machine_name == 'resource') {
    $question = t('Are you sure you want to delete the resource <em>@name</em>?',
      array('@name' => trim($entity->firstname . ' ' . $entity->lastname)));
    $path = 'resource/' . $resource_id . '/edit';
  }
  else {
    global $language;
    
    $resource = resource_load($resource_id);
    $attr = array(
      '@name' => trim($resource->firstname . ' ' . $resource->lastname),
      '@record' => $section->details[$language->language]['title'],
    );
    $question = t('Are you sure you want to delete the record <em>@record</em> for resource <em>@name</em>?', $attr);
    $path = 'resource/' . $resource_id . '/' . $section->machine_name . '/' . $id . '/edit';
  }
  
  $form = array();
  $form['#entity'] = $entity;
  $form['#section'] = $section;
  $form['#resource_id'] = $resource_id;
  
  return confirm_form($form, $question, $path);
}

function resource_delete_form_submit($form, &$form_state) {
  $entity = $form['#entity'];
  $resource_id = $form['#resource_id'];
  $section = $form['#section'];
  
  if ($section->machine_name == 'resource') {
    $attr = array(
      '@name' => trim($entity->firstname . ' ' . $entity->lastname),
    );
    if (resource_delete($entity)) {
      drupal_set_message(t('The <em>@name</em> resource has been deleted successfully', $attr));
      $form_state['redirect'] = 'resource';
    }
    else {
      drupal_set_message(t('Failed to delete the resource <em>@name</em>', $attr), 'error');
    }
  }
  else {
    global $language;
    $resource = resource_load($resource_id);
    $attr = array(
      '@name' => trim($resource->firstname . ' ' . $resource->lastname),
      '@record' => $section->details[$language->language]['title'],
    );
    if (section_entity_delete($section, $entity)) {
      drupal_set_message(t('The record <em>@record</em> of resource <em>@name</em> has been deleted successfully', $attr));
      $form_state['redirect'] = 'resource/' . $resource_id . '/' . $section->machine_name;
    }
    else {
      drupal_set_message(t('Failed to delete the record <em>@record</em> for resource <em>@name</em>', $attr), 'error');
    }
  }
}

function resource_view_form($form_state, $section_machine_name, $resource_id) {
  $section = section_load($section_machine_name);
  if (empty($section)) {
    drupal_not_found();
    exit();
  }
  
  $resource = resource_load($resource_id);
  drupal_set_title(trim($resource->firstname . ' ' . $resource->lastname));
  $breadcrumb = array(
    l(t('Home'), '<front>'),
    l(t('Resources'), 'resource'),
  );
  drupal_set_breadcrumb($breadcrumb);
  
  if ($section->type == SECTION_TYPE_ONE_TO_ONE) {
    return resource_view_single_form($form_state, $section, $resource_id);
  }
  else {
    return resource_view_grid_form($form_state, $section, $resource_id);
  }
}

function resource_view_single_form($form_state, $section, $resource_id) {
  $entity = section_entity_load($section, $resource_id);
  if (empty($entity) && empty($session->remote_key)) {
    drupal_not_found();
    exit();
  }
  
  $form = array();
  
  $links = array();
  if ($section->machine_name == 'resource') {
    if (resource_check_access('edit', $section->machine_name, $resource_id)) {
      $links[] = l(t('Edit'), 'resource/' . $resource_id . '/edit');
    }
  }
  elseif (empty($entity)) {
    if (resource_check_access('add', $section->machine_name, $resource_id)) {
      $links[] = l(t('Edit'), 'resource/' . $resource_id . '/' . $section->machine_name . '/add');
    }
  }
  else {
    if (resource_check_access('edit', $section->machine_name, $resource_id)) {
      $links[] = l(t('Edit'), 'resource/' . $resource_id . '/' . $section->machine_name . '/0/edit');
    }
  }
  
  if (!empty($links)) {
    $form['actions'] = array(
      '#type' => 'fieldset',
      '#title' => t('Actions'),
    );
    
    $form['actions']['links'] = array(
      '#value' => theme('item_list', $links),
    );
  }
  
  $fields = section_load_fields($section, TRUE);
  $categories = array();
  foreach ($fields as $field) {
    $categories[$field->category][] = $field->machine_name;
  }
  
  $placeholders = array(
    '@entityid' => $resource_id,
    '@recordid' => 0,
  );
  
  foreach ($categories as $category => $field_names) {
    if (!empty($category)) {
      $form[$category] = array(
        '#type' => 'fieldset',
        '#title' => $category,
      );
    }
    
    foreach ($field_names as $machine_name) {
      $field = $fields[$machine_name];
      $form[$category][$field->machine_name] = array(
        '#type' => 'item',
        '#title' => $field->title,
        '#weight' => $field->weight,
        '#value' => section_format_value($field, isset($entity->{$field->machine_name}) ? $entity->{$field->machine_name} : NULL, $placeholders),
      );
    }
  }
  
  return $form;
}

function resource_view_grid_form($form_state, $section, $resource_id) {
  $form = array();
  $header = array();
  $rows = array();
  $extension = resource_extension_load($section->id);
  if (empty($extension)) {
    $extension = (object) array(
      'orientation' => RESOURCE_ORIENTATION_VERTICAL,
      'order_by_field' => '',
      'order_direction' => 'asc',
    );
  }
  
  $fields = section_load_fields($section, TRUE);
  $entities = section_entity_load($section, $resource_id, 0, $extension->order_by_field, $extension->order_direction);
  if ($extension->orientation == RESOURCE_ORIENTATION_VERTICAL) {
    foreach ($fields as $field) {
      $header[$field->machine_name] = array('data' => $field->title);
    }
    
    foreach ($entities as $entity) {
      $placeholders = array(
        '@entityid' => $resource_id,
        '@recordid' => $entity->id,
      );
      
      $row = array();
      foreach ($header as $key => $value) {
        $field = $fields[$key];
        $row[$key] = section_format_value($field, isset($entity->$key) ? $entity->$key : NULL, $placeholders);
      }
      $row[] = l(t('Edit'), 'resource/' . $resource_id . '/' . $section->machine_name . '/' . $entity->id . '/edit');
      
      $rows[] = $row;
    }
    
    $header[] = '';
    if (empty($rows)) {
      $rows[] = array(
        array('data' => t('No items found.'), 'colspan' => count($header)),
      );
    }
  }
  else {
    foreach ($fields as $field) {
      $rows[$field->machine_name] = array(array('data' => $field->title, 'header' => TRUE));
    }
    $rows[''] = array('');
    
    foreach ($entities as $entity) {
      $placeholders = array(
        '@entityid' => $resource_id,
        '@recordid' => $entity->id,
      );
      
      foreach ($fields as $field) {
        $key = $field->machine_name;
        $rows[$key][] = section_format_value($field, isset($entity->$key) ? $entity->$key : NULL, $placeholders);
      }
      $rows[''][] = l(t('Edit'), 'resource/' . $resource_id . '/' . $section->machine_name . '/' . $entity->id . '/edit');
    }
  }
  
  $links = array();
  $links[] = l(t('Add'), 'resource/' . $resource_id . '/' . $section->machine_name . '/add');
  
  $form['actions'] = array(
    '#type' => 'fieldset',
    '#title' => t('Actions'),
  );
  
  $form['actions']['links'] = array(
    '#value' => theme('item_list', $links),
  );
  
  $form['result'] = array(
    '#value' => theme('table', $header, $rows),
  );
  
  return $form;
}

function resource_extend_section_form($form_state, $section) {
  $extension = resource_extension_load($section->id);
  if (empty($extension)) {
    $extension = (object) array(
      'section_id' => 0,
      'orientation' => 'none',
      'order_by_field' => '',
      'order_direction' => 'asc',
    );
  }
  
  drupal_set_title(t('Edit section <em>@name</em>', array('@name' => $section->machine_name)));
  
  $form = array();
  $form['#section'] = $section;
  $form['#extension'] = $extension;
  
  $form['orientation'] = array(
    '#type' => 'select',
    '#title' => t('Orientation'),
    '#options' => array(
      'none' => t('None'),
      RESOURCE_ORIENTATION_VERTICAL => t('Vertical'),
      RESOURCE_ORIENTATION_HORIZONTAL => t('Horizontal'),
    ),
    '#default_value' => $extension->orientation,
  );
  
  $list = array('' => t('- None -'));
  $fields = section_load_fields($section);
  foreach ($fields as $field) {
    $list[$field->machine_name] = $field->title;
  }
  $form['order_by_field'] = array(
    '#type' => 'select',
    '#title' => t('Sort by'),
    '#description' => t("Field's machine name"),
    '#options' => $list,
    '#default_value' => $extension->order_by_field,
  );
  
  $form['order_direction'] = array(
    '#type' => 'select',
    '#title' => t('Sort direction'),
    '#options' => array(
      'asc' => t('Ascendent'),
      'desc' => t('Descendent'),
    ),
    '#default_value' => $extension->order_direction,
  );
  
  $form['actions'] = array(
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );

  $form['actions']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  $form['actions']['cancel'] = array(
    '#value' => l(t('Cancel'), 'admin/build/section/' . $section->id . '/edit'),
  );

  return $form;
}

function resource_extend_section_form_submit($form, &$form_state) {
  $section = $form['#section'];
  $extension = $form['#extension'];
  
  if (empty($extension->section_id)) {
    $add = TRUE;
    $extension->section_id = $section->id;
  }
  else {
    $add = FALSE;
  }
  $extension->orientation = $form_state['values']['orientation'];
  $extension->order_by_field = $form_state['values']['order_by_field'];
  $extension->order_direction = $form_state['values']['order_direction'];
  
  if ($form_state['values']['orientation'] == 'none') {
    $result = db_query("DELETE FROM {resource_sections} WHERE section_id = %d", $section->id);
  }
  else {
    $result = drupal_write_record('resource_sections', $extension, $add ? array() : 'section_id');
  }
  
  if ($result !== FALSE) {
    drupal_set_message(t('Data has been saved successfully'));
  }
  else {
    drupal_set_message(t('Failed to save data'), 'error');
  }
}
