function UnHide( eThis ){
	var icon = eThis.childNodes[0].getAttribute('src');
        var plus = /icons\/plus.gif/;
	if( plus.test(icon) ){
            eThis.childNodes[0].setAttribute('src', 'icons/minus.gif');
            eThis.parentNode.parentNode.parentNode.className = '';
        }else{
            eThis.childNodes[0].setAttribute('src', 'icons/plus.gif');
            eThis.parentNode.parentNode.parentNode.className = 'cl';
        }
        return false;
    }
