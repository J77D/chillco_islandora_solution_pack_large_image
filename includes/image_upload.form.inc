<?php

/**
 * @file
 *
 * Handles the uploading of the image file.
 */

/**
 * Defines a file upload form for uploading the basic image.
 *
 * @param array $form
 *   The drupal form.
 * @param array $form_state
 *   The drupal form state.
 * @return
 *   The drupal form definition.
 */
function chillco_islandora_solution_pack_large_image_image_upload_form(array $form, array &$form_state) {
  $upload_size = min((int)ini_get('post_max_size'), (int)ini_get('upload_max_filesize'));
  $extensions = array('tiff tif jpg jpeg png gif');

  $form['original_image'] = array(
    '#title' => t('Image'),
    '#type' => 'managed_file',
    '#required' => FALSE,
    '#description' => t('Select image to upload.<br/>Files must be less than <b>!size MB.</b><br/>Recommended image file type: <b>!ext.</b>', array('!size' => $upload_size, '!ext' => $extensions[0])),
    '#default_value' => isset($form_state['values']['files']) ? $form_state['values']['files'] : NULL,
    '#upload_location' => 'temporary://',
    '#upload_validators' => array(
      'file_validate_extensions' => $extensions,
      'file_validate_size' => array($upload_size * 1024 * 1024), // Assume its specified in MB
    ),
  );

  return $form;
}

/**
 * Submit handler, adds uploaded file to ingest object
 *
 * @param array $form
 *   The drupal form.
 * @param array $form_state
 *   The drupal form state.
 */
function chillco_islandora_solution_pack_large_image_image_upload_form_submit(array $form, array &$form_state) {
  global $user;

  $object = $form_state['islandora']['objects'][0];
  $original_image = NULL;
  if (!empty($form_state['values']['original_image'])) {
    // Save the original image into the OBJ datastream.
    $original_image = $form_state['values']['original_image'];
    if (empty($object['OBJ'])) {
      $original_image_ds = $object->constructDatastream('OBJ', 'M');
      $object->ingestDatastream($original_image_ds);
    }
    else {
      $original_image_ds = $object['OBJ'];
    }
    $file = file_load($form_state['values']['original_image']);
    $file->source = $file->uri;
    $file->ops = array();
    $path = drupal_realpath($file->uri);
    $original_image_ds->setContentFromFile($path);
    $original_image_ds->label = $file->filename;
    $original_image_ds->mimetype = $file->filemime;

    // Save from the OBJ datastream to the IMAGE datastream.
    $path = pathinfo(drupal_realpath($file->uri));
    $image_file_path = 'temporary://' . $path['filename'] . '.IMAGE.jpeg';
    $status = image_imagemagick_save($file, $image_file_path);
    if ($status) {
      $image_file = new stdClass();
      $image_file->fid = NULL;
      $image_file->uid = $user->uid;
      $image_file->filename = $path['filename'] . '.IMAGE.jpeg';
      $image_file->uri = $image_file_path;
      $image_file->filemime = file_get_mimetype($image_file->uri);
      $image_file->status = 0;
      if ($image_file = file_save($image_file)) {
        $form_state['values']['lossy_image'] = $image_file->fid;
      }
    }
    if (empty($object['IMAGE'])) {
      $image_ds = $object->constructDatastream('IMAGE', 'M');
      $object->ingestDatastream($image_ds);
    }
    else {
      $image_ds = $object['IMAGE'];
    }
    $file = file_load($form_state['values']['lossy_image']);
    $path = drupal_realpath($file->uri);
    $image_ds->setContentFromFile($path);
    $image_ds->label = $file->filename;
    $image_ds->mimetype = $file->filemime;
  }
}
