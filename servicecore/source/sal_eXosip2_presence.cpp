

#include "sal_eXosip2.h"
#include "ECMedia.h"

extern "C" char *osip_call_id_new_random();
//#include "eXosip2.h"

typedef enum {
	PIDF = 0,
	RFCxxxx = 1,
	MSOLDPRES = 2
} presence_type_t;

/*
 * REVISIT: this static variable forces every dialog to use the same presence description type depending 
 * on what is received on a single dialog...
 */
static presence_type_t presence_style = PIDF;

SalOp * sal_find_out_subscribe(Sal *sal, int sid){
	const MSList *elem;
	SalOp *op;
	for(elem=sal->out_subscribes;elem!=NULL;elem=elem->next){
		op=(SalOp*)elem->data;
		if (op->sid==sid) return op;
	}
	WriteLogToFile("No op for sid %i\n",sid);
	return NULL;
}

static void sal_add_out_subscribe(Sal *sal, SalOp *op){
	sal->out_subscribes=ms_list_append(sal->out_subscribes,op);
}

void sal_remove_out_subscribe(Sal *sal, SalOp *op){
	sal->out_subscribes=ms_list_remove(sal->out_subscribes,op);
}

SalOp * sal_find_in_subscribe(Sal *sal, int nid){
	const MSList *elem;
	SalOp *op;
	for(elem=sal->in_subscribes;elem!=NULL;elem=elem->next){
		op=(SalOp*)elem->data;
		if (op->nid==nid) return op;
	}
	return NULL;
}

static SalOp * sal_find_in_subscribe_by_call_id(Sal *sal, osip_call_id_t *call_id){
	const MSList *elem;
	SalOp *op;
	for(elem=sal->in_subscribes;elem!=NULL;elem=elem->next){
		op=(SalOp*)elem->data;
		if (op->call_id && osip_call_id_match(op->call_id,call_id)==0)
			return op;
	}
	return NULL;
}

static void sal_add_in_subscribe(Sal *sal, SalOp *op, osip_message_t *subs){
	osip_call_id_clone(subs->call_id,&op->call_id);
	sal->in_subscribes=ms_list_append(sal->in_subscribes,op);
}

void sal_remove_in_subscribe(Sal *sal, SalOp *op){
	sal->in_subscribes=ms_list_remove(sal->in_subscribes,op);
}



char *sal_text_send(SalOp *op, const char *from, const char *to, const char *msg){
	osip_message_t *sip=NULL;
    
    char *callid = NULL;
	if(op->cid == -1)
	{
		/* we are not currently in communication with the destination */
		if (from)
			sal_op_set_from(op,from);
		if (to)
			sal_op_set_to(op,to);

		sal_exosip_fix_route(op);
		eXosip_lock();
		eXosip_message_build_request(&sip,"MESSAGE",sal_op_get_to(op),
			sal_op_get_from(op),sal_op_get_route(op));
		if (sip!=NULL){
			osip_message_set_content_type(sip,"text/plain");
			osip_message_set_body(sip,msg,strlen(msg));
			sal_add_other(op->base.root,op,sip);
			eXosip_message_send_request(sip);
            osip_call_id_to_str(sip->call_id, &callid);
		}else{
			WriteLogToFile("Could not build MESSAGE request !\n");
		}
		eXosip_unlock();
	}
	else
	{
		/* we are currently in communication with the destination */
		eXosip_lock();
		//First we generate an INFO message to get the current call_id and a good cseq
		eXosip_call_build_request(op->did,"MESSAGE",&sip);
		if(sip == NULL)
		{
			WriteLogToFile("could not get a build info to send MESSAGE, maybe no previous call established ?\n");
			eXosip_unlock();
			return NULL;
		}
		osip_message_set_content_type(sip,"text/plain");
		osip_message_set_body(sip,msg,strlen(msg));
		eXosip_call_send_request(op->did,sip);
        osip_call_id_to_str(sip->call_id, &callid);
		eXosip_unlock();
	}
    
	return callid;
}

void sal_get_call_id(char **callid)
{
    osip_call_id_t *call_id;
    char *cidrand;
    
    /* call-id is always the same for REGISTRATIONS */
    int ret = osip_call_id_init(&call_id);
    if (ret != 0) {
        *callid = NULL;
        return;
    }
    char *strCallID = NULL;
    cidrand = osip_call_id_new_random();
    osip_call_id_set_number(call_id, cidrand);
    osip_call_id_to_str(call_id, &strCallID);
    osip_free(call_id);
    osip_free(cidrand);
    
    *callid = strCallID;
}

/*presence Subscribe/notify*/
int sal_subscribe_presence(SalOp *op, const char *from, const char *to){
	osip_message_t *msg=NULL;
	if (from)
		sal_op_set_from(op,from);
	if (to)
		sal_op_set_to(op,to);
	sal_exosip_fix_route(op);
	eXosip_lock();
	eXosip_subscribe_build_initial_request(&msg,sal_op_get_to(op),sal_op_get_from(op),
	    	sal_op_get_route(op),"presence",600);
	if (msg==NULL){
		WriteLogToFile("Could not build subscribe request to %s\n",to);
		eXosip_unlock();
		return -1;
	}
	if (op->base.contact){
		_osip_list_set_empty(&msg->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(msg,op->base.contact);
	}
	op->sid=eXosip_subscribe_send_initial_request(msg);
	eXosip_unlock();
	if (op->sid==-1){
		osip_message_free(msg);
		return -1;
	}
	sal_add_out_subscribe(op->base.root,op);
	return 0;
}

int sal_unsubscribe(SalOp *op){
	osip_message_t *msg=NULL;
	if (op->did==-1){
		WriteLogToFile("cannot unsubscribe, no dialog !\n");
		return -1;
	}
	eXosip_lock();
	eXosip_subscribe_build_refresh_request(op->did,&msg);
	if (msg){
		osip_message_set_expires(msg,"0");
		eXosip_subscribe_send_refresh_request(op->did,msg);
	}else WriteLogToFile("Could not build subscribe refresh request ! op->sid=%i, op->did=%i\n",
	    	op->sid,op->did);
	eXosip_unlock();
	return 0;
}

int sal_subscribe_accept(SalOp *op){
	osip_message_t *msg=NULL;
	eXosip_lock();
	eXosip_insubscription_build_answer(op->tid,202,&msg);
	if (msg==NULL){
		WriteLogToFile("Fail to build answer to subscribe.\n");
		eXosip_unlock();
		return -1;
	}
	if (op->base.contact){
		_osip_list_set_empty(&msg->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(msg,op->base.contact);
	}
	eXosip_insubscription_send_answer(op->tid,202,msg);
	eXosip_unlock();
	return 0;
}

int sal_subscribe_decline(SalOp *op){
	eXosip_lock();
	eXosip_insubscription_send_answer(op->tid,401,NULL);
	eXosip_unlock();
	return 0;
}

static void mk_presence_body (const SalPresenceStatus online_status, const char *contact_info,
		char *buf, size_t buflen, presence_type_t ptype) {
  switch (ptype) {
    case RFCxxxx: {
	  /* definition from http://msdn.microsoft.com/en-us/library/cc246202%28PROT.10%29.aspx */
	  int atom_id = 1000;

	  if (online_status==SalPresenceOnline)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\" priority=\"0.800000\">\n\
<status status=\"open\" />\n\
<msnsubstatus substatus=\"online\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status == SalPresenceBusy ||
			  online_status == SalPresenceDonotdisturb)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\" priority=\"0.800000\">\n\
<status status=\"inuse\" />\n\
<msnsubstatus substatus=\"busy\" />\n\
</address>\n\
</atom>\n</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status==SalPresenceBerightback)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\" priority=\"0.800000\">\n\
<status status=\"open\" />\n\
<msnsubstatus substatus=\"berightback\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status == SalPresenceAway ||
			  online_status == SalPresenceMoved)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\" priority=\"0.800000\">\n\
<status status=\"open\" />\n\
<msnsubstatus substatus=\"away\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status==SalPresenceOnthephone)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\" priority=\"0.800000\">\n\
<status status=\"inuse\" />\n\
<msnsubstatus substatus=\"onthephone\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status==SalPresenceOuttolunch)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\" priority=\"0.800000\">\n\
<status status=\"open\" />\n\
<msnsubstatus substatus=\"outtolunch\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence PUBLIC \"-//IETF//DTD RFCxxxx XPIDF 1.0//EN\" \"xpidf.dtd\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\" priority=\"0.800000\">\n\
<status status=\"closed\" />\n\
<msnsubstatus substatus=\"away\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);
	  }
	  break;
    } 
    case MSOLDPRES: {
	/* Couldn't find schema http://schemas.microsoft.com/2002/09/sip/presence
 	*  so messages format has been taken from Communigate that can send notify
 	*  requests with this schema
 	*/
	  int atom_id = 1000;

	  if (online_status==SalPresenceOnline)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence SYSTEM \"http://schemas.microsoft.com/2002/09/sip/presence\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\">\n\
<status status=\"open\" />\n\
<msnsubstatus substatus=\"online\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status == SalPresenceBusy ||
			  online_status == SalPresenceDonotdisturb)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence SYSTEM \"http://schemas.microsoft.com/2002/09/sip/presence\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\">\n\
<status status=\"inuse\" />\n\
<msnsubstatus substatus=\"busy\" />\n\
</address>\n\
</atom>\n</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status==SalPresenceBerightback)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence SYSTEM \"http://schemas.microsoft.com/2002/09/sip/presence\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\">\n\
<status status=\"inactive\" />\n\
<msnsubstatus substatus=\"berightback\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status == SalPresenceAway ||
			  online_status == SalPresenceMoved)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence SYSTEM \"http://schemas.microsoft.com/2002/09/sip/presence\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\">\n\
<status status=\"inactive\" />\n\
<msnsubstatus substatus=\"idle\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status==SalPresenceOnthephone)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence SYSTEM \"http://schemas.microsoft.com/2002/09/sip/presence\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\">\n\
<status status=\"inuse\" />\n\
<msnsubstatus substatus=\"onthephone\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else if (online_status==SalPresenceOuttolunch)
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence SYSTEM \"http://schemas.microsoft.com/2002/09/sip/presence\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\">\n\
<status status=\"inactive\" />\n\
<msnsubstatus substatus=\"outtolunch\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);

	  }
	  else
	  {
		  snprintf(buf, buflen, "<?xml version=\"1.0\"?>\n\
<!DOCTYPE presence SYSTEM \"http://schemas.microsoft.com/2002/09/sip/presence\">\n\
<presence>\n\
<presentity uri=\"%s;method=SUBSCRIBE\" />\n\
<atom id=\"%i\">\n\
<address uri=\"%s\">\n\
<status status=\"closed\" />\n\
<msnsubstatus substatus=\"offline\" />\n\
</address>\n\
</atom>\n\
</presence>", contact_info, atom_id, contact_info);
	  }
	break;
	}
    default: { /* use pidf+xml as default format, rfc4479, rfc4480, rfc3863 */

	if (online_status==SalPresenceOnline)
	{
		snprintf(buf, buflen, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" \
xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" \
xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" \
entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status><basic>open</basic></status>\n\
<contact priority=\"0.8\">%s</contact>\n\
</tuple>\n\
</presence>",
contact_info, contact_info);
	}
	else if (online_status == SalPresenceBusy ||
			online_status == SalPresenceDonotdisturb)
	{
		snprintf(buf, buflen, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" \
xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" \
xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" \
entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status><basic>open</basic></status>\n\
<contact priority=\"0.8\">%s</contact>\n\
</tuple>\n\
<dm:person id=\"sg89aep\">\n\
<rpid:activities><rpid:busy/></rpid:activities>\n\
</dm:person>\n\
</presence>",
contact_info, contact_info);
	}
	else if (online_status==SalPresenceBerightback)
	{
		snprintf(buf, buflen, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" \
xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" \
xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" \
entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status><basic>open</basic></status>\n\
<contact priority=\"0.8\">%s</contact>\n\
</tuple>\n\
<dm:person id=\"sg89aep\">\n\
<rpid:activities><rpid:in-transit/></rpid:activities>\n\
</dm:person>\n\
</presence>",
contact_info, contact_info);
	}
	else if (online_status == SalPresenceAway ||
			online_status == SalPresenceMoved)
	{
		snprintf(buf, buflen, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" \
xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" \
xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" \
entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status><basic>open</basic></status>\n\
<contact priority=\"0.8\">%s</contact>\n\
</tuple>\n\
<dm:person id=\"sg89aep\">\n\
<rpid:activities><rpid:away/></rpid:activities>\n\
</dm:person>\n\
</presence>",
contact_info, contact_info);
	}
	else if (online_status==SalPresenceOnthephone)
	{
		snprintf(buf, buflen, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" \
xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" \
xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" \
entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status><basic>open</basic></status>\n\
<contact priority=\"0.8\">%s</contact>\n\
</tuple>\n\
<dm:person id=\"sg89aep\">\n\
<rpid:activities><rpid:on-the-phone/></rpid:activities>\n\
</dm:person>\n\
</presence>",
contact_info, contact_info);
	}
	else if (online_status==SalPresenceOuttolunch)
	{
		snprintf(buf, buflen, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" \
xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" \
xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" \
entity=\"%s\">\n\
<tuple id=\"7777\">\n\
<status><basic>open</basic></status>\n\
<contact priority=\"0.8\">%s</contact>\n\
</tuple>\n\
<dm:person id=\"78787878\">\n\
<rpid:activities><rpid:meal/></rpid:activities>\n\
<rpid:note>Out to lunch</rpid:note> \n\
</dm:person>\n\
</presence>",
contact_info, contact_info);
	}
	else
	{
		snprintf(buf, buflen, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<presence xmlns=\"urn:ietf:params:xml:ns:pidf\" \
xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\" \
xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\" \
entity=\"%s\">\n\
<tuple id=\"sg89ae\">\n\
<status><basic>closed</basic></status>\n\
<contact priority=\"0.8\">%s</contact>\n\
</tuple>\n\
</presence>\n", contact_info, contact_info);
	}
	break;
    }
 } // switch

}

static void add_presence_body(osip_message_t *notify, SalPresenceStatus online_status)
{
	char buf[1000];
	char *contact_info;

	osip_from_t *from=NULL;
	from=osip_message_get_from(notify);
	osip_uri_to_str(from->url,&contact_info);

	mk_presence_body (online_status, contact_info, buf, sizeof (buf), presence_style);

	osip_message_set_body(notify, buf, strlen(buf));
	osip_message_set_content_type(notify,
		presence_style ? "application/xpidf+xml" : "application/pidf+xml");

	osip_free(contact_info);
}


int sal_notify_presence(SalOp *op, SalPresenceStatus status, const char *status_message){
	osip_message_t *msg=NULL;
	eXosip_ss_t ss=EXOSIP_SUBCRSTATE_ACTIVE;
	if (op->nid==-1){
		WriteLogToFile("Cannot notify, subscription was closed.\n");
		return -1;
	}
	
	eXosip_lock();
	eXosip_insubscription_build_notify(op->did,ss,DEACTIVATED,&msg);
	if (msg!=NULL){
		const char *identity=sal_op_get_contact(op);
		if (identity==NULL) identity=sal_op_get_to(op);
		_osip_list_set_empty(&msg->contacts,(void (*)(void*))osip_contact_free);
		osip_message_set_contact(msg,identity);
		add_presence_body(msg,status);
		eXosip_insubscription_send_request(op->did,msg);
	}else WriteLogToFile("could not create notify for incoming subscription.\n");
	eXosip_unlock();
	return 0;
}

int sal_notify_close(SalOp *op){
	osip_message_t *msg=NULL;
	eXosip_lock();
	eXosip_insubscription_build_notify(op->did,EXOSIP_SUBCRSTATE_TERMINATED,DEACTIVATED,&msg);
	if (msg!=NULL){
		const char *identity=sal_op_get_contact(op);
		if (identity==NULL) identity=sal_op_get_to(op);
		osip_message_set_contact(msg,identity);
		add_presence_body(msg,SalPresenceOffline);
		eXosip_insubscription_send_request(op->did,msg);
	}else WriteLogToFile("sal_notify_close(): could not create notify for incoming subscription\n"
	    " did=%i, nid=%i",op->did,op->nid);
	eXosip_unlock();
	return 0;
}

int sal_publish(SalOp *op, const char *from, const char *to, SalPresenceStatus presence_mode){
	osip_message_t *pub;
	int i;
	char buf[1024];

	mk_presence_body (presence_mode, from, buf, sizeof (buf), presence_style);

	i = eXosip_build_publish(&pub,from, to, NULL, "presence", "300", 
		presence_style ? "application/xpidf+xml" : "application/pidf+xml", buf);
	if (i<0){
		WriteLogToFile("Failed to build publish request.\n");
		return -1;
	}

	eXosip_lock();
	i = eXosip_publish(pub, to); /* should update the sip-if-match parameter
				    from sip-etag  from last 200ok of PUBLISH */
	eXosip_unlock();
	if (i<0){
	  WriteLogToFile("Failed to send publish request.\n");
	  return -1;
	}
	sal_add_other(sal_op_get_sal(op),op,pub);
	return 0;
}

static void _sal_exosip_subscription_recv(Sal *sal, eXosip_event_t *ev){	
	SalOp *op=sal_op_new(sal);
	char *tmp;
	op->did=ev->did;
	op->tid=ev->tid;
	op->nid=ev->nid;
	osip_from_to_str(ev->request->from,&tmp);
	sal_op_set_from(op,tmp);
	ms_free((void **)&tmp);
	osip_from_to_str(ev->request->to,&tmp);
	sal_op_set_to(op,tmp);
	ms_free((void **)&tmp);
	sal_add_in_subscribe(sal,op,ev->request);
	sal->callbacks.subscribe_received(op,sal_op_get_from(op));
}

void sal_exosip_subscription_recv(Sal *sal, eXosip_event_t *ev){	
	/*workaround a bug in eXosip: incoming SUBSCRIBES within dialog with expires: 0 are
	 recognized as new incoming subscribes*/
	SalOp *op=sal_find_in_subscribe_by_call_id(sal,ev->request->call_id);
	if (op){
		osip_header_t *h;
		osip_message_header_get_byname(ev->request,"expires",0,&h);
		if (h && h->hvalue && atoi(h->hvalue)==0){
			WriteLogToFile("This susbscribe is not a new one but terminates an old one.\n");
			ev->did=op->did;
			ev->nid=op->nid;
			sal_exosip_subscription_closed(sal,ev);
		}else {
			osip_message_t *msg=NULL;
			WriteLogToFile("Probably a refresh subscribe\n");
			eXosip_lock();
			eXosip_insubscription_build_answer(ev->tid,202,&msg);
			eXosip_insubscription_send_answer(ev->tid,202,msg);
			eXosip_unlock();
		}
	}else _sal_exosip_subscription_recv(sal,ev);
}

void sal_exosip_notify_recv(Sal *sal, eXosip_event_t *ev){
	SalOp *op=sal_find_out_subscribe(sal,ev->sid);
	char *tmp;
	osip_from_t *from=NULL;
	osip_body_t *body=NULL;
	SalPresenceStatus estatus=SalPresenceOffline;
	
	WriteLogToFile("Receiving notify with sid=%i,nid=%i\n",ev->sid,ev->nid);

	if (op==NULL){
		WriteLogToFile("No operation related to this notify !\n");
		return;
	}
	if (ev->request==NULL) return;

	from=ev->request->from;
	osip_message_get_body(ev->request,0,&body);
	if (body==NULL){
		WriteLogToFile("No body in NOTIFY\n");
		return;
	}
	osip_from_to_str(from,&tmp);
	if (strstr(body->body,"pending")!=NULL){
		estatus=SalPresenceOffline;
	}else if (strstr(body->body,"busy")!=NULL){
		estatus=SalPresenceBusy;
	}else if (strstr(body->body,"berightback")!=NULL
			|| strstr(body->body,"in-transit")!=NULL ){
		estatus=SalPresenceBerightback;
	}else if (strstr(body->body,"away")!=NULL
			|| strstr(body->body,"idle")){
		estatus=SalPresenceAway;
	}else if (strstr(body->body,"onthephone")!=NULL
		|| strstr(body->body,"on-the-phone")!=NULL){
		estatus=SalPresenceOnthephone;
	}else if (strstr(body->body,"outtolunch")!=NULL
			|| strstr(body->body,"meal")!=NULL){
		estatus=SalPresenceOuttolunch;
	}else if (strstr(body->body,"closed")!=NULL){
		estatus=SalPresenceOffline;
	}else if ((strstr(body->body,"online")!=NULL) || (strstr(body->body,"open")!=NULL)) {
		estatus=SalPresenceOnline;
	}else{
		estatus=SalPresenceOffline;
	}
	WriteLogToFile("We are notified that %s has online status %i\n",tmp,estatus);
	if (ev->ss_status==EXOSIP_SUBCRSTATE_TERMINATED) {
		sal_remove_out_subscribe(sal,op);
		op->sid=-1;
		op->did=-1;
		WriteLogToFile("And outgoing subscription terminated by remote.\n");
	}
	sal->callbacks.notify_presence(op,op->sid!=-1 ? SalSubscribeActive : SalSubscribeTerminated, estatus,NULL);

	/* try to detect presence message style used by server,
 	 * and switch our presence messages to servers style */
	if (strstr (body->body, "//IETF//DTD RFCxxxx XPIDF 1.0//EN") != NULL) {
		presence_style = RFCxxxx;
	} else if (strstr(body->body,"http://schemas.microsoft.com/2002/09/sip/presence")!=NULL) {
		presence_style = MSOLDPRES;
	}
	
	osip_free(tmp);
}

void sal_exosip_subscription_answered(Sal *sal,eXosip_event_t *ev){
	SalOp *op=sal_find_out_subscribe(sal,ev->sid);
	if (op==NULL){
		WriteLogToFile("Subscription answered but no associated op !\n");
		return;
	}
	op->did=ev->did;
}

void sal_exosip_in_subscription_closed(Sal *sal, eXosip_event_t *ev){
	SalOp *op=sal_find_in_subscribe(sal,ev->nid);
	char *tmp;
	if (op==NULL){
		WriteLogToFile("Incoming subscription closed but no associated op !\n");
		return;
	}
	
	
	sal_remove_in_subscribe(sal,op);
	op->nid=-1;
	op->did=-1;
	if (ev->request){
		osip_from_to_str(ev->request->from,&tmp);
		sal->callbacks.subscribe_closed(op,tmp);
		osip_free(tmp);
	}
}

void sal_exosip_subscription_closed(Sal *sal,eXosip_event_t *ev){
	SalOp *op=sal_find_out_subscribe(sal,ev->sid);
	if (op==NULL){
		WriteLogToFile("Subscription closed but no associated op !\n");
		return;
	}
	sal_remove_out_subscribe(sal,op);
	op->sid=-1;
	op->did=-1;
	sal->callbacks.notify_presence(op,SalSubscribeTerminated, SalPresenceOffline,NULL);
}

