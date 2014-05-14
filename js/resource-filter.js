Drupal.behaviors.resource_filter = function(context) {
  $('#resource-filter-form').submit(function () {
    var data = [];
    
    $('#resource-filter-form tr.row').each(function () {
      var val1 = '';
      var val2 = '';
      
      $(this).find('td.value1 select')
      
      var item = {
        operator: $(this).find('td.operator select').val(),
        attribute: $(this).find('td.attribute select').val(),
        condition: $(this).find('td.condition select').val(),
        value1: val1,
        value2: val2
      };
      data.push(item);
    });
    
    $('#edit-data').val(JSON.stringify(data));
  });
  
  $('#resource-filter-form a.add-row').click(function () {
    var row = $('#resource-filter-form tr.drow')
      .clone()
      .removeClass('drow')
      .addClass('row')
      .show();
    
    row.find('td.delete a').click(function () {
      if ($('#resource-filter-form tr.row').length > 1) {
        $(this).parent().parent().remove();
      }
      return false;
    });
    
    row.find('td.attribute select').change(function () {
      var row = $(this).parent().parent().parent();
      var fieldId = $(this).val();
      var field = Drupal.settings.resource.fields[fieldId];
      
      // Preload value
      if (Drupal.settings.resource.fields[fieldId]['value'] == '') {
        $.getJSON(Drupal.settings.resource.ajaxUrl, {fid: fieldId}, function (data) {
          if (data.status) {
            Drupal.settings.resource.fields[data.fieldId]['value'] = data.value;
            
            // Adjust condition
            var selectedCondition = 0;
            row.find('td.condition select option').hide();
            for (x in Drupal.settings.resource.conditions) {
              if (Drupal.settings.resource.conditions[x]['category'] == field.searchable) {
                if (!selectedCondition) {
                  selectedCondition = x;
                }
                var condition = Drupal.settings.resource.conditions[x];
                row.find('td.condition select option[value=' + condition.id + ']').show();
              }
            }
            row.find('td.condition select').val(selectedCondition).trigger('change');
          }
        });
      }
      else {
        // Adjust condition
        var selectedCondition = 0;
        row.find('td.condition select option').hide();
        for (x in Drupal.settings.resource.conditions) {
          if (Drupal.settings.resource.conditions[x]['category'] == field.searchable) {
            if (!selectedCondition) {
              selectedCondition = x;
            }
            var condition = Drupal.settings.resource.conditions[x];
            row.find('td.condition select option[value=' + condition.id + ']').show();
          }
        }
        row.find('td.condition select').val(selectedCondition).trigger('change');
      }
    });
    
    row.find('td.condition select').change(function () {
      var row = $(this).parent().parent().parent();
      var conditionId = $(this).val();
      var fieldId = row.find('td.attribute select').val();
      var field = Drupal.settings.resource.fields[fieldId];
      var condition = Drupal.settings.resource.conditions[conditionId];
      
      switch (parseInt(condition.num_args)) {
        case 1:
          row.find('td.value1').html(field.value);
          row.find('td.value2').html('');
          break;
        case 2:
          row.find('td.value1').html(field.value);
          row.find('td.value2').html(field.value);
          break;
        default:
          row.find('td.value1').html('');
          row.find('td.value2').html('');
      }
    });
    
    row.insertBefore($('#resource-filter-form tr.drow'));
    row.find('td.attribute select').trigger('change');
    return false;
  }).trigger('click');
};
