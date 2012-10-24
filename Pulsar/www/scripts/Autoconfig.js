function checkSelectedClients(clientName)
{
  for(var i in selectedClients)
  {
    if(selectedClients[i] == clientName)
    {
      return true;
    }
  }
  return false;
}
    
function setTarget(source, target, group)
{
  if(group == 'true' || group == true)
  {
    if(document.getElementById(source).checked)
    {
      message.id = target + ':';
      var targets = target.split(':');
      
      while(document.getElementById('autoconfElements').hasChildNodes())
      {
        document.getElementById('autoconfElements').removeChild(document.getElementById('autoconfElements').lastChild);
      }
      
      for(var i in targets)
      {
        if(targets != undefined && targets != '')
        {
          createAutoconfigElement(targets[i]);
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
      while(document.getElementById('autoconfElements').hasChildNodes())
      {
        document.getElementById('autoconfElements').removeChild(document.getElementById('autoconfElements').lastChild);
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
      var targets = target.split(':');
      for(var i in targets)
      {
        if(targets[i] != undefined && targets[i] != '')
        {
          createAutoconfigElement(targets[i]);
        }
      }
    }
    else
    {
      var regex = new RegExp(target + ':', 'i');
      message.id = message.id.replace(regex, '');
      
      var targets = target.split(':');
      for(var i in targets)
      {
        if(targets[i] != undefined && targets[i] != '')
        {
          removeAutoconfigElement(targets[i]);
        }
      }
    }
  }
}

function changeLabel(type, index)
{
  if(type == 'ratio')
  {
    document.getElementById('change' + index).innerHTML = ' times the amount of real nodes found on ';
  }
  else
  {
    document.getElementById('change' + index).innerHTML = ' nodes on '; 
  }
}

function createAutoconfigElement(clientName, group)
{
  selectedClients.push(clientName);
  var tr = document.createElement('tr');
  tr.id = 'autohook' + clientName;
  tr.setAttribute('style', 'background: #E8A02F;');
  var td0 = document.createElement('td');
  var label = document.createElement('label');
  label.value = clientName;
  label.innerHTML = clientName + ': Create ';
  var inputType = document.createElement('select');
  inputType.id = 'type' + elementCount;
  var types = ['number', 'ratio'];
  for(var i = 0; i < 2; i++)
  {
    var option = document.createElement('option');
    option.value = types[i];
    option.innerHTML = types[i];
    inputType.appendChild(option);
  }
  inputType.setAttribute('onclick', 'changeLabel(document.getElementById("type' + elementCount + '").value, ' + elementCount + ')');
  
  var input = document.createElement('input');
  input.type = 'number';
  input.min = '0';
  //This needs to be found dynamically
  input.max = '1024';
  input.value = '0';
  //dropDown needs to be a dojo select containing a list of the interfaces on the 
  //host with clientId clientName
  var label1 = document.createElement('label');
  label1.innerHTML = ' nodes on ';
  label1.id = 'change' + elementCount;
  var dropDown = document.createElement('select');
  now.GetInterfacesOfClient(clientName, function(interfaces){
    for(var i in interfaces)
    {
      var option = document.createElement('option');
      option.value = interfaces[i];
      option.innerHTML = interfaces[i];
      dropDown.appendChild(option);
    }
  });
  td0.appendChild(label);
  td0.appendChild(inputType);
  td0.appendChild(input);
  td0.appendChild(label1);
  td0.appendChild(dropDown);
  tr.appendChild(td0);
  if(group != '')
  {
    document.getElementById('autoconfElements').appendChild(tr);
  }
  else
  {
    document.getElementById('autoconfElements').appendChild(tr);
  }
  document.getElementById('sendAutoconfig').removeAttribute('disabled');
}

function removeAutoconfigElement(target)
{
  var loopIter = document.getElementById('autoconfElements');
  do 
  {
    if(document.getElementById('autohook' + target) != undefined)
    {
      loopIter.removeChild(document.getElementById('autohook' + target));
      var regex = new RegExp(target + ':', 'i');
      message.id = message.id.replace(regex, '');
      break;
    }
  }while(loopIter.hasChildNodes());
  
  for(var j in selectedClients)
  {
    if(selectedClients[j] == target)
    {
      delete selectedClients[j];
    }
  }
}

function processAutoconfigElements()
{
  var loopIter = document.getElementById('autoconfElements');
  do 
  {
    if(loopIter.lastChild != undefined)
    {
      var simple = loopIter.lastChild.childNodes[0];
      console.log('Constructing message');
      var autoconfMessage = {};
      autoconfMessage.type = 'haystackConfig';
      autoconfMessage.id = simple.childNodes[0].value;
      autoconfMessage.numNodesType = simple.childNodes[1].value;
      autoconfMessage.numNodes = simple.childNodes[2].value;
      if(/^[0-9]+$/i.test(autoconfMessage.numNodes.toString()) == false)
      {
        simple.childNodes[2].value = '0';
        alert('Number of nodes/ratio of nodes to create must be a digit');
        return;
      }
      autoconfMessage.interface = simple.childNodes[4].value;
      now.MessageSend(autoconfMessage);
    }
    loopIter.removeChild(loopIter.lastChild);
  }while(loopIter.hasChildNodes());
}