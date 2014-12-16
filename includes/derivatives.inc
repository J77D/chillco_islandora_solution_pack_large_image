<?php

/**
 * Creates large jpg derivative from IMAGE datastream.
 */
function chillco_islandora_solution_pack_large_image_create_large_derivative(AbstractObject $object, $force = FALSE) {
  return chillco_islandora_solution_pack_large_image_create_jpg_derivative($object, $force, 'LARGE', 'chillco_islandora_solution_pack_large_image_large');
}

/**
 * Creates medium jpg derivative from IMAGE datastream.
 */
function chillco_islandora_solution_pack_large_image_create_medium_derivative(AbstractObject $object, $force = FALSE) {
  return chillco_islandora_solution_pack_large_image_create_jpg_derivative($object, $force, 'MEDIUM', 'chillco_islandora_solution_pack_large_image_medium');
}

/**
 * Creates tn jpg derivative from IMAGE datastream.
 */
function chillco_islandora_solution_pack_large_image_create_tn_derivative(AbstractObject $object, $force = FALSE) {
  return chillco_islandora_solution_pack_large_image_create_jpg_derivative($object, $force, 'TN', 'chillco_islandora_solution_pack_large_image_tn');
}

/**
 * Creates a jpg datastream derivative of the IMAGE datastream using a Image
 * Styles preset.
 */
function chillco_islandora_solution_pack_large_image_create_jpg_derivative(AbstractObject $object, $force, $dsid, $style_name) {
  $to_return = array(
    'success' => FALSE,
    'messages' => array(),
  );
  $base_name = str_replace(':', '-', $object->id);
  $uploaded_file = chillco_islandora_solution_pack_large_image_get_uploaded_image_file($object, $base_name);
  if ($uploaded_file && ($force || !isset($object[$dsid]))) {
    $filename = file_create_filename("{$base_name}.{$dsid}.jpg", 'temporary://');
    $image_style = image_style_load($style_name);
    if(image_style_create_derivative($image_style, $uploaded_file, $filename)) {
      $file_path = drupal_realpath($filename);
      if (chillco_islandora_solution_pack_large_image_add_datastream($object, $dsid, $file_path, file_get_mimetype($file_path), basename($file_path))) {
        $to_return['success'] = TRUE;
        $to_return['messages'][] = array(
          'message' => t('Successfully created derivative for @dsid', array('@dsid' => $dsid)),
          'type' => 'watchdog',
          'severity' => WATCHDOG_INFO,
        );
      }
      else {
        $to_return['messages'][] = array(
          'message' => t('Could not create derivative for @dsid: could not attach datastream to @id', array('@dsid' => $dsid, '@id' => $object->id)),
          'type' => 'dsm',
          'severity' => 'error',
        );
      }
    }
    else {
      $to_return['messages'][] = array(
        'message' => t('Could not create derivative for @dsid: derivative image style was not created', array('@dsid' => $dsid)),
        'type' => 'dsm',
        'severity' => 'error',
      );
    }
    file_unmanaged_delete($filename);
  }
  else {
    $to_return['messages'][] = array(
      'message' => t('Could not create derivative for @dsid: uploaded file does not exist and/or (regeneration was not forced and datastream already exists)', array('@dsid' => $dsid)),
      'type' => 'dsm',
      'severity' => 'error',
    );
  }
  file_unmanaged_delete($uploaded_file);
  return $to_return;
}

/**
 * Gets the IMAGE datastream associated with an object.
 */
function chillco_islandora_solution_pack_large_image_get_uploaded_image_file(AbstractObject $object, $base_name) {
  $convert = FALSE;
  if (isset($object['IMAGE'])) {
    $current_object = $object['IMAGE'];
  }
  else if (isset($object['OBJ'])) {
    $convert = TRUE;
    $current_object = $object['OBJ'];
  }
  else {
    return FALSE;
  }
  $mime_detector = new MimeDetect();
  $ext = $mime_detector->getExtension($current_object->mimeType);
  $filename = file_create_filename("{$base_name}_ORIGINAL.{$ext}", 'temporary://');
  $current_object->getContent($filename);
  // Create conversion and ingest.
  if ($convert) {
    $new_filename = file_create_filename("{$base_name}.IMAGE.jpg", 'temporary://');
    _imagemagick_convert($filename, $new_filename, array());
    file_unmanaged_delete($filename);
    $filename = $new_filename;
    $file_path = drupal_realpath($filename);
    chillco_islandora_solution_pack_large_image_add_datastream($object, 'IMAGE', $file_path, file_get_mimetype($file_path), basename($file_path));
  }
  return $filename;
}

/**
 * Adds datastream to object.
 */
function chillco_islandora_solution_pack_large_image_add_datastream(AbstractObject $object, $dsid, $file, $mimetype, $label) {
  $ds = $object->constructDatastream($dsid, 'M');
  $ds->label = $label;
  $ds->mimeType = $mimetype;
  $ds->setContentFromFile($file);
  try {
    if (isset($object[$dsid])) {
      $object->purgeDatastream($dsid);
    }
    $object->ingestDatastream($ds);
  } catch (exception $e) {
    drupal_set_message(t('@message', array('@message' => check_plain($e->getMessage()))), 'error');
    return FALSE;
  }
  return TRUE;
}
