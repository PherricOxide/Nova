function watermark(id, text)
{
  var element = document.getElementById(id);

  if(element.value.length > 0)
  {
    if(element.value == text)
    {
      element.value = '';
      if(id.indexOf('date') != -1)
      {
        element.setAttribute('style', 'width: 120px;');
      }
      else
      {
        element.removeAttribute('style'); 
      }
    }
  }
  else
  {
    element.value = text;
    if(id.indexOf('date') != -1)
    {
      element.setAttribute('style', 'font-style: italic; width: 120px;');
    }
    else
    {
      element.setAttribute('style', 'font-style: italic;');
    }
  }
}

function setTarget(source, target, group)
{
  if(group == 'true' || group == true)
  {
    while(document.getElementById('elementHook').hasChildNodes())
    {
      document.getElementById('elementHook').removeChild(document.getElementById('elementHook').lastChild);
    }
    
    if(document.getElementById(source).checked)
    {
      message.id = target + ':';
      var targets = target.split(':');

      for(var i in targets)
      {
        if(targets[i] != '' && targets[i] != undefined)
        {
          createScheduledEventElement(targets[i]);
        } 
      }

      for(var i in document.getElementById('groupsList').childNodes)
      {
        if(document.getElementById('groupcheck' + i) != undefined && document.getElementById('groupcheck' + i).id.indexOf(source) == -1)
        {
          document.getElementById('groupcheck' + i).setAttribute('disabled', true);
        }
      }
      for(var i in document.getElementById('clientsList').childNodes)
      {
        if(document.getElementById('check' + i) != undefined)
        {
          document.getElementById('check' + i).checked = false;
          document.getElementById('check' + i).setAttribute('disabled', true);
        } 
      }
    }
    else
    {
      message.id = '';
      var targets = target.split(':');
      
      while(document.getElementById('elementHook').hasChildNodes())
      {
        document.getElementById('elementHook').removeChild(document.getElementById('elementHook').lastChild);
      }
      for(var i in document.getElementById('groupsList').childNodes)
      {
        if(document.getElementById('groupcheck' + i) != undefined && document.getElementById('groupcheck' + i).id.indexOf(source) == -1)
        {
          document.getElementById('groupcheck' + i).removeAttribute('disabled');
        }
      }
      for(var i in document.getElementById('clientsList').childNodes)
      {
        if(document.getElementById('check' + i) != undefined)
        {
          document.getElementById('check' + i).removeAttribute('disabled');
        } 
      }
    }
  }
  else
  {
    if(document.getElementById(source).checked)
    {
      message.id += target + ':';
      createScheduledEventElement(target);
    }
    else
    {
      var regex = new RegExp(target + ':', 'i');
      message.id = message.id.replace(regex, '');
      document.getElementById('elementHook').removeChild(document.getElementById(target));
    }
  }
}

function createScheduledEventElement(clientId)
{
  var borderDiv = document.createElement('div');
  borderDiv.id = clientId;
  borderDiv.setAttribute('style', 'border: 2px solid; background: #E8A02F; width: 450px;');
  
  var label0 = document.createElement('h1');
  label0.setAttribute('style', 'text-align: center');
  label0.innerHTML = clientId;
  
  var label1 = document.createElement('label');
  label1.innerHTML = 'Message Type: ';
  
  var typeSelect = document.createElement('select');
  typeSelect.id = clientId + 'select';
  
  for(var i in messageTypes)
  {
    if(messageTypes[i] != '' && messageTypes[i] != undefined)
    {
      var option = document.createElement('option');
      option.value = messageTypes[i];
      option.innerHTML = messageTypes[i];
      typeSelect.appendChild(option);
    }
  }
  
  typeSelect.setAttribute('onclick', 'newTypeSelected("' + clientId + '")');

  var label5 = document.createElement('label');
  label5.innerHTML = 'Cron String: ';
  
  var cron = document.createElement('input');
  cron.id = clientId + 'cron';

  var exLabel = document.createElement('label');
  exLabel.innerHTML = '(W, L and # are not supported for the cron syntax. Use 0 for Sunday, not 7)';
  exLabel.setAttribute('style', 'font-style: italic');

  var label6 = document.createElement('label');
  label6.innerHTML = 'OR once on ';
  
  var date = document.createElement('input');
  date.id = clientId + 'date';
  date.value = 'Date MM/DD/YYYY';
  date.setAttribute('style', 'font-style: italic; width: 120px');
  date.setAttribute('onblur', 'watermark("' + date.id + '", "Date MM/DD/YYYY")');
  date.setAttribute('onfocus', 'watermark("' + date.id + '", "Date MM/DD/YYYY")');
  
  var time = document.createElement('input');
  time.id = clientId + 'time';
  time.value = 'Time(24hr) HH:MM(:SS)';
  time.setAttribute('style', 'font-style: italic');
  time.setAttribute('onblur', 'watermark("' + time.id + '", "Time(24hr) HH:MM(:SS)")');
  time.setAttribute('onfocus', 'watermark("' + time.id + '", "Time(24hr) HH:MM(:SS)")');

  var label4 = document.createElement('label');
  label4.innerHTML = 'Event Name: ';
  
  var name = document.createElement('input');
  name.id = clientId + 'name';
  name.maxlength = '10';
  
  var b0 = document.createElement('br');
  var b2 = document.createElement('br');
  var b3 = document.createElement('br');
  var b4 = document.createElement('br');
  var b5 = document.createElement('br');
  var b6 = document.createElement('br');
  
  borderDiv.appendChild(label0);
  borderDiv.appendChild(label1);
  borderDiv.appendChild(typeSelect);
  borderDiv.appendChild(b0);
  borderDiv.appendChild(label5);
  borderDiv.appendChild(cron);
  borderDiv.appendChild(b3);
  borderDiv.appendChild(exLabel);
  borderDiv.appendChild(b5);
  borderDiv.appendChild(label6);
  borderDiv.appendChild(date);
  borderDiv.appendChild(time);
  borderDiv.appendChild(b6);
  borderDiv.appendChild(label4);
  borderDiv.appendChild(name);
  
  document.getElementById('elementHook').appendChild(borderDiv);
}

function newTypeSelected(clientId)
{
  // For when message types requiring arguments are supported
}

function registerScheduledMessage(clientId, name, message, cron, date, cb)
{
  message.id = clientId + ':';
  now.SetScheduledMessage(clientId, name, message, cron, date, cb);
}

function submitSchedule()
{
  for(var i in document.getElementById('elementHook').childNodes)
  {
    var id = document.getElementById('elementHook').childNodes[i].id; 
    
    if(/^[a-z0-9]+$/i.test(document.getElementById(id + 'name').value) == false/* || document.getElementById(id + 'name').value.length <= 1*/)
    {
      document.getElementById(id + 'name').value = '';
      alert('Scheduled event names must consist of 2-10 alphanumeric characters');
      return;
    }
    
    if(id != undefined)
    {
      if((document.getElementById(id + 'name').value != '')
      && (document.getElementById(id + 'cron').value != ''))
      {
        var name = document.getElementById(id + 'name').value;
        
        var registerMessage = {};
        registerMessage.type = document.getElementById(id + 'select').value;
        
        var cron = document.getElementById(id + 'cron').value;
        
        if(/^[a-z0-9\*\/\-]+$/i.test(cron) == false)
        {
          document.getElementById(id + 'cron').value = '';
          alert('cron string contains unallowed characters!');
          return;
        }
        
        registerScheduledMessage(id, name, registerMessage, cron, undefined, function(id, status, response){
          if(status == 'failed')
          {
            console.log(response);
          }
          else if(status == 'succeeded')
          {
            console.log(response);
            document.getElementById('elementHook').removeChild(document.getElementById(id));
            document.getElementById(id + 'div').childNodes[0].checked = false; 
          }
          else
          {
            console.log('Unknown status received from SetScheduledMessage, doing nothing.');
          }  
        });
      }
      else if((document.getElementById(id + 'date').value != '')
      && (document.getElementById(id + 'time').value != '')
      && (document.getElementById(id + 'cron').value == '')
      && (document.getElementById(id + 'name').value != ''))
      {
        var temp = new Date();
        
        if(new RegExp('^[0-9]{2}\/[1-3]?[0-9]\/[0-9]{4}$', 'i').test(document.getElementById(id + 'date').value) == false)
        {
          alert('Improperly formatted date string');
          return;
        }
        
        var monthdayyear = document.getElementById(id + 'date').value.split('/');
        var month = parseInt(monthdayyear[0]) - 1;
        
        if(month < 0 || month > 11)
        {
          alert('Invalid value for month');
          return; 
        }
        
        var day = parseInt(monthdayyear[1]);
        
        if(monthdayyear[1].length == 2 && monthdayyear[1] == '0')
        {
          day = parseInt(day[1]); 
        }
        else if(monthdayyear[1].length > 2 || (day < 1 || day > 31))
        {
          alert('Invalid day value for MM/DD/YYYY string');
          return; 
        }
        
        var year = parseInt(monthdayyear[2]);
        
        if(monthdayyear[2].length != 4)
        {
          alert('Year value for MM/DD/YYYY string must be four characters');
          return; 
        }
        else if(/^[0-1][0-9]\:[0-5]?[0-9](\:[0-5]?[0-9])?$/.test(document.getElementById(id + 'time').value) == false)
        {
          alert('Improperly formatted time string');
          return;
        }
        
        var hourminutesecond = document.getElementById(id + 'time').value.split(':');
        var hour = (parseInt(hourminutesecond[0]) % 24);
        var minute = parseInt(hourminutesecond[1]);
        var second = parseInt((hourminutesecond[2] != undefined ? hourminutesecond[2] : 0));
        
        if(isNaN(month) || isNaN(day) || isNaN(year)
        || isNaN(hour) || isNaN(minute) || isNaN(second))
        {
          alert('Define a cron string for recurring events or a one time run date/time'); 
        }
        
        var name = document.getElementById(id + 'name').value;
        
        var registerMessage = {};
        registerMessage.type = document.getElementById(id + 'select').value;
        
        var date = new Date(year, month, day, hour, minute, second);
        
        registerScheduledMessage(id, name, registerMessage, undefined, date, function(id, status, response){
          if(status == 'failed')
          {
            console.log(response);
          }
          else if(status == 'succeeded')
          {
            console.log(response);
            document.getElementById('elementHook').removeChild(document.getElementById(id));
            document.getElementById(id + 'div').childNodes[0].checked = false; 
          }
          else
          {
            console.log('Unknown status received from SetScheduledMessage, doing nothing.');
          }  
        });
      }
      else if(document.getElementById(id + 'name').value == '')
      {
        console.log('Scheduled event must have a name.'); 
      }
      else if(document.getElementById(id + 'cron').value == '')
      {
        console.log('Scheduled event must have a cron string');
      }
    }
  }
}