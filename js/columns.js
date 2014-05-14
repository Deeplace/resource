Drupal.behaviors.resource_columns = function(context) {
  $('#edit-move-right').click(function (event) {
    move_ids($('#edit-all-columns').val());
    update_selected();
    $('#edit-all-columns').val(0);
    
    return false;
  });
  
  $('#edit-move-left').click(function (event) {
    var ids = $('#edit-selected-columns').val();
    
    for (var x in ids) {
      $("#edit-all-columns option[value='" + ids[x] + "']").show();
      $("#edit-selected-columns option[value='" + ids[x] + "']").remove();
    }
    
    update_selected();
    $("#edit-selected-columns").val(0);
    
    return false;
  });
  
  $('#edit-move-up').click(function (event) {
    $('#edit-selected-columns option:selected').each(function () {
      $(this).prev().before($(this)); 
    });
    
    update_selected();
    
    return false;
  });
  
  $('#edit-move-down').click(function (event) {
    $('#edit-selected-columns option:selected').each(function () {
      $(this).next().after($(this)); 
    });
    
    update_selected();
    
    return false;
  });
  
  $('#resource-set-columns-form').submit(function () {
    $('#edit-selected-columns').attr('name', '');
  });
  
  move_ids(Drupal.settings.resource.selectedColumns);
  update_selected();
};

function move_ids(ids) {
  for (var x in ids) {
    $("#edit-all-columns option[value='" + ids[x] + "']").css('display', 'none');
    
    $('#edit-selected-columns')
      .append($("<option></option>")
      .attr("value", ids[x])
      .text(Drupal.settings.resource.columns[ids[x]])); 
  }
}

function update_selected() {
  var val = [];
  
  $('#edit-selected-columns option').each(function () {
    val.push($(this).attr('value')); 
  });
  
  $('#edit-selected').val(val.join(', '));
}
