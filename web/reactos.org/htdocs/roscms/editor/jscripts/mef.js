
	var roscmseditorsavecache = '';

	function fileBrowserCallBack(field_name, url, type, win) {
		// This is where you insert your custom filebrowser logic
		alert("Filebrowser callback: field_name: " + field_name + ", url: " + url + ", type: " + type);

		// Insert new URL, this would normaly be done in a popup
		win.document.forms[0].elements[field_name].value = "someurl.htm";
	}
	
	function toggleEditor(id, objid) {
		var elm = document.getElementById(id);
		var butid = document.getElementById(objid);
		var wrapid = document.getElementById('swraped'+objid.substr(6));
		
		if (tinyMCE.getInstanceById(id) == null) {
			tinyMCE.execCommand('mceAddControl', false, id);
			butid.value = 'HTML Source';
			wrapid.style.visibility = 'hidden';
		}
		else {
			tinyMCE.execCommand('mceRemoveControl', false, id);
			elm.style.backgroundColor = '#FFFFFF';
			butid.value = 'Rich Text';
			wrapid.style.visibility = 'visible';
		}
		elm = null;
	}
	
	function ajaxsaveContent(id) {
		//alert('#'+id+'#');
		
		if (tinyMCE.getInstanceById(id) != null) {
//			alert('content::: '+tinyMCE.getInstanceById(id).getHTML());
			return tinyMCE.getInstanceById(id).getHTML();
		}
		return null;
	}
	
	
	function setWrap(val, objid) {
		var s = document.getElementById(objid);
	
		s.wrap = val;
	
		var agt=navigator.userAgent.toLowerCase();
	
		if (agt.indexOf('gecko')>=0 || agt.indexOf('opera')>=0) {
			var v = s.value;
			var n = s.cloneNode(false);
			n.setAttribute("wrap", val);
			s.parentNode.replaceChild(n, s);
			n.value = v;
		}
	}
	
	function toggleWordWrap(elm, objid) {
		if (document.getElementById(elm).checked)
			setWrap('soft', objid);
		else
			setWrap('off', objid);
	}
	
	
	function ajaxLoad() {
		var inst = tinyMCE.getInstanceById('elm1');
	
		// Do you ajax call here
		inst.setHTML('HTML content that got passed from server.');
	}
	
	function ajaxSave() {
		var inst = tinyMCE.getInstanceById('elm1');
	
		// Do you ajax call here
		alert(inst.getHTML());
	}