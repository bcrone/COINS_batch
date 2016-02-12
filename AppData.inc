<?php
    /* app specific data.
       Really only needs to by an abstraction of stored procedures and
       functions.
       Base class will run generic select queries for you. */

    require_once('CAS.php');
    require_once('BaseAppData.inc');
    require_once('asmt/classes/EvalQuestion.inc');

    class AppData extends BaseAppData {

      function isValidSubjectID($id){
         // check if user exists
         return $this->getLoginFromSubjectID($id);
      }
      function getLoginFromSubjectID($subject_id)
      {
         $qryStr = "SELECT l.login_id,
                           l.study_id,
                           l.usid,
                           l.source_type,
                           l.nonstop_queue,
                           d.sa_review_emails,
                           d.study_label as label,
                           d.gender,
                           d.age,
                           d.subject_type_id 
                    FROM mrsdba.mrs_subject_logins_vw l 
                    JOIN mrs_subject_details_active_vw d
                        ON l.study_id = d.study_id AND l.usid = d.usid
                    WHERE subject_id='$subject_id'";

         $rs = $this->select($qryStr);

         /* if return is error, return an error string to caller */
         if(PEAR::isError($rs)) {
            return get_class($this)." getLoginFromSubjectID():".$rs->getMessage();
         }

         if(sizeof($rs) == 0) return null;
         return $rs[0];
      }

      /*
      * DEPRECATED use the ORM subjectAssessments queue class instead
      */
      function getSubjectAssessmentQueues($login_id){
         //returns an array representing the first items in each of the subject's queues
         // no assessments in queue for empty login:
         if(!$login_id) return array();

         $qryStr = "select distinct on (queue_type_id) aqueue_id,
                           i.instrument_id,
                           i.label as instrument_label,
                           q.assessment_id,
                           sort_order,
                           q.queue_type_id,
                           qt.sa_label as queue_type_label,
                           qt.sa_description as queue_type_description,
                           qc.queue_length
                    from mrsdba.mrs_subject_assessments_queue_vw q
                    inner join mrsdba.mrs_inst_vw i
                    on i.instrument_id = q.instrument_id
                    inner join mrs_subject_assessments_queue_types_vw qt
                    on qt.queue_type_id = q.queue_type_id
                    inner join (SELECT count(*) as queue_length, queue_type_id
                        FROM mrs_subject_assessments_queue_vw
                        where login_id = $login_id
                        group by queue_type_id) as qc
                    on qc.queue_type_id = qt.queue_type_id
                    where login_id=$login_id ";
         $qryStr .= " order by queue_type_id, sort_order asc";


         $rs = $this->select($qryStr);


         /* if return is error, return an error string to caller */
         if(PEAR::isError($rs)) {
            return get_class($this)." getSubjectAssessmentQueue():".$rs->getMessage().' '.$qryStr;
         }
         return $rs;

      }

      /*
      * DEPRECATED use the ORM subjectAssessments queue class instead
      */
      function getSubjectAssessmentQueue($login_id, $queuetype = null){
         // no assessments in queue for empty login:
         if(!$login_id) return array();

         $qryStr = "select aqueue_id,
                           i.instrument_id,
                           i.label as instrument_label,
                           q.assessment_id,
                           sort_order
                    from mrsdba.mrs_subject_assessments_queue_vw q
                    inner join mrsdba.mrs_inst_vw i
                    on i.instrument_id = q.instrument_id
                    where login_id=$login_id ";
         if($queuetype != null && $queuetype != ''){
            $qryStr .= " and queue_type_id = $queuetype ";
         }
         $qryStr .= " order by sort_order asc";


         $rs = $this->select($qryStr);


         /* if return is error, return an error string to caller */
         if(PEAR::isError($rs)) {
            error_log('SA Queue Error 1:'.$rs->getMessage());
            return get_class($this)." getSubjectAssessmentQueue():".$rs->getMessage();
         }
         elseif(!count($rs)){
            error_log ('SA Queue Warning 1: empty Queue. Query = '.$qryStr);
         }
         return $rs;

      }
        /* Returns recordset of subject enrollment information
         * for an USID, study, and subject type
         * only for subjects where the User has privilege to see enrollment information
         * if $include_history = true then include disenrollment history as well
         */
        function getSubjectEnrollmentByUSID($usid, $study_id='all',
                                            $subject_type_id='all',
                                            $include_history='false') {



            // start query
            $qryStr = "select
                         sub.usid, sub.ursi, sub.subject_type_id, sub.type_label,
                         sub.study_id, sub.study_label, sub.ethnicity,
                            to_char(sub.enrolled_date,'MM/DD/YYYY') as enrolled_date,
                            sub.user_enrolled, study.hrrc_num,
                         sub.agreestosharedata,
                         study.irb_number, study.pi_id,sub.disenrolled_date,
                         to_char(sub.consent_date,'MM/DD/YYYY') AS consent_date,
                         isphilinked, sub.notes
                   from mrs_subject_details_vw sub,
                        mrs_studies_vw study
                       where
                         sub.study_id = study.study_id
                       and
                         sub.usid = '" . $usid . "'";
         if( $include_history=='false' ) {
            $qryStr.= " and sub.disenrolled_date is NULL ";
         }

         if( $study_id!='all'){
            $qryStr = $qryStr." and sub.study_id = $study_id";
         }

            if( $subject_type_id != 'all' ) {
               $qryStr .= " and sub.subject_type_id = " . $subject_type_id . " ";
            }
            /* Changing the 'order by' will break the enrollment editor
             * (micis/subjects/enrollmenteditor.inc).
             * The recodset must be ordered by enrolled_date first so that the
             * enrollment editor knows which record to highlight as red, indicating
             * the 'first enrollment' for the subject.
             * This feature helps users track subjects only by the
             * 'First URSI'.
             */
            if( $include_history=='true' ) {
            $qryStr = $qryStr." order by sub.enrolled_date,
                                         sub.disenrolled_date,
                                         sub.study_label";
            } else {
            $qryStr = $qryStr." order by sub.enrolled_date,sub.study_label";
         }

         $rs = $this->select( $qryStr );
         /* return recordset or PEAR error object */
         return $rs;

      }

      function getStudyDetails($study_id) {

         $qryStr = "
               SELECT
               st.study_id,
               st.label,
               st.hrrc_num,
               st.irb_number,
               st.approved_sessions,
               st.remaining_sessions,
               st.description,
               st.hide_savenexit_in_sa,
               to_char(st.expiration_date,'MM/DD/YYYY') as expiration_date,
               st.pi_id,
               st.co_pi_id,
               st.rad_review_emails,
               st.primary_research_area_id,
               st.secondary_research_area_id,
               ss.label as status_label,
               ss.description as status_description,
               ss.status_id,
               pi.first_name,
               pi.last_name,
               pi2.first_name as co_pi_first_name,
               pi2.last_name as co_pi_last_name,
               st.exp_warn_emails,
               to_char(st.hrrc_consent_date,'MM/DD/YYYY') as hrrc_consent_date,
               st.grant_number,
               st.url_reference,
               st.url_description,
               st.sponsor,
               st.hrrc_title,
               ra1.label as primary_research_area,
               ra2.label as secondary_research_area,
               st.add_to_recruitment,
               st.recruitment_study_purpose,
               st.recruitment_protocol_summary,
               st.recruitment_contact_name,
               st.recruitment_contact_email,
               st.recruitment_contact_phone,
               st.site_id,
               st.max_enrollment,
               st.\"count\" as current_enrollment,
               st.reuse_ursi,
               st.nonstop_queue_in_sa
               FROM
                   mrs_pi_vw pi
                   INNER JOIN mrs_studies_with_enrollment_vw st
                   ON st.pi_id = pi.person_id
                   INNER JOIN mrs_study_statuses_vw ss
                   ON st.status_id = ss.status_id
                   LEFT OUTER JOIN mrs_study_research_area_vw ra1
                   ON ra1.study_research_area_id = st.primary_research_area_id
                   LEFT OUTER JOIN mrs_study_research_area_vw ra2
                   ON st.secondary_research_area_id = ra2.study_research_area_id
                   LEFT OUTER JOIN mrs_pi_vw pi2
                   ON pi2.person_id = st.co_pi_id
               WHERE
                  st.study_id = $study_id
         ";

         $rs = $this->select( $qryStr );

         if(PEAR::isError($rs)) {
            return $this->raiseError(get_class($this)." <br> Getting study details<br> ".$rs->getMessage());
         }

         /* return recordset or PEAR error object */
         return $rs[0];

      }
      function getSubjectAssessmentQueueItem($queueInstrumentID){
         $qryStr = "select instrument_id,
                           assessment_id,
                           segment_interval_id,
                           queue_type_id,
                           sort_order
                    from mrsdba.mrs_subject_assessments_queue_vw q
                    where queue_instrument_id = $queueInstrumentID";


         $rs = $this->select($qryStr);

         /* if return is error, return an error string to caller */
         if(PEAR::isError($rs)) {
            return get_class($this)." getSubjectAssessmentQueueItem():".$rs->getMessage();
         }

         if(!$rs) return null;
         return $rs[0];
      }

        /*
           +----------------------------------------------------------------------+
           | getInstrument($study_id,$instrument_id=0)
           | study_id required                                                    |
           +----------------------------------------------------------------------+
           | returns record set                                                   |
           | if error, returns error string                                       |
           +----------------------------------------------------------------------+
        */
        function getInstrument($study_id,$instrument_id=0) {


         if($study_id==-99) {
            $qryString="select i.instrument_id, i.label ,i.salabel, i.description,
                               i.version, i.display_type, i.max_per_segment, i.hide_sa_previous
                               '(' ||s.study_id || ') ' || s.label as study,
                               s.study_id
                        from MRS_INST_STUDIES_VW i, mrs_studies_vw s
                        where i.is_owner=1
                        and i.study_id= s.study_id
                        order by i.label, i.description
            ";
         } else {
            // security not enforced here assuming that active study id is never revealed or changable by the user.
            if($instrument_id == 0) {
               $qryString = "select * from MRS_INST_STUDIES_VW where study_id = $study_id order by LABEL";
            }
            elseif($instrument_id == -1) {
               $qryString = "select * from MRS_INST_STUDIES_VW where study_id = $study_id and (is_hidden != 1 or is_hidden is null) order by LABEL";
            }
            else {
               $qryString = "select * from MRS_INST_STUDIES_VW where study_id = $study_id and instrument_id = $instrument_id order by LABEL";
            }
         }

         /* simple select statement */
         $rs = $this->select($qryString);

         /* if return is error, return an error string to caller */
         if(PEAR::isError($rs)) {
            return get_class($this)." getInstrument() ".$rs->getMessage();
         }
         /* return record set to caller */
         else {
            return $rs;
         }

        }
        /*
           +----------------------------------------------------------------------+
           | isValidUrsi()                                                        |
           +----------------------------------------------------------------------+
           | checks if ursi is subject table                                                          |
             | returns true if it is, return false otherwise                        |
           +----------------------------------------------------------------------+
        */
        function isValidStudyUrsi($ursi,$study_id) {



                    $qryString = "select ursi from MRS_SUBJECT_TYPE_DETAILS_VW " .
                                 "where ursi = '".strtoupper($ursi)."' " .
                                 "and study_id = $study_id";

                    /* simple select statement */
                    $rs = $this->select($qryString);

                    /* if return is error, return an error string to caller */
                    if(PEAR::isError($rs)) {
                        return 'false';
                    }
                    /* return record set to caller */
                    else {
                        if( (is_array($rs))  &&  (count($rs)) ) {
                            return 'true';
                        }
                        else {
                            return 'false';
                        }
                    }
        }
        /*
           +----------------------------------------------------------------------+
           | startAssessment()                                                    |
           +----------------------------------------------------------------------+
           |                                            |
           +----------------------------------------------------------------------+
        */
        function startAssessment(
           $study_id,
           $instrument_id,
           $assessment_date,
           $assessment_starttime,
           $ursi,
           $rater1,
           $rater2=null,
           $source_type,
           $site_id,
           $dataentry_type_id,
           $segment_interval,
           $force_first_entry,
           $isSelfAssessment=false,
           $singleEntry=false,
           $ongoingEntry=false,
           $auto_queue_generated=false,
           $from_protocol_id=null
        ) {
           $assessment_date_time = $assessment_date.' '.$assessment_starttime . ':00';
           if ($rater2 == null || $rater2=='') $rater2=null;
           if ($isSelfAssessment == null || $isSelfAssessment=='') $isSelfAssessment=null;
           $inParam = array();
           $inParam[] = $study_id;
           $inParam[] = $instrument_id;
           $inParam[] = $assessment_date_time;
           $inParam[] = $ursi;
           $inParam[] = $rater1;
           $inParam[] = $rater2;
           $inParam[] = $source_type;
           $inParam[] = $site_id;
           $inParam[] = $dataentry_type_id;
           $inParam[] = $segment_interval;
           $inParam[] = $isSelfAssessment;
           $inParam[] = $force_first_entry=='T' ? "true" : "false";
           $inParam[] = $singleEntry ? "true" : "false";
           $inParam[] = $ongoingEntry ? "true" : "false";
           $inParam[] = $auto_queue_generated ? "true" : "false";
           $inParam[] = $from_protocol_id;
           $inParam[] = $this->db_transaction_username;
           $rs = $this->funCall('mrs_start_assessment_f', $inParam);

           if ( $this->isError($rs) ) {
               if (preg_match('/MAX_INSTANCES_EXCEEDED/', $rs->message)) {
                   print '<h3>Sorry, we cannot proceed to the next ' .
                       'questionnaire because you have already completed ' .
                       'it.  Please contact study staff for assistance.</h3>';
                   exit;
               } else {
                   return $this->raiseError(get_class($this).":".__FUNCTION__." "
                   .$rs->message);
               }
           }

           /* retval will have two values delimited by a pipe
              parse that here and return an array.
                This will allow error detection on array status
                */
           $aRetval = array();
           $aRetval['assessment_id'] = strtok($rs,"|");
           $aRetval['entry_code'] = strtok("|");

           return $aRetval;
        }
      function updateAssessmentQueueItem($queueInstrumentID, $assessment_id){
         // enter new queue
         $inParam = array();
         $inParam[] = $queueInstrumentID;
         $inParam[] = $assessment_id;
         $inParam[] = $this->db_transaction_username;

         $rs = $this->funCall('mrsdba.mrs_update_subj_ass_queue_item_f', $inParam);
         return $rs;
      }
        /*
           +----------------------------------------------------------------------+
           | updateAssessmentProperties()                                         |
           +----------------------------------------------------------------------+
           | write one or more changes to assessment event record                 |
           +----------------------------------------------------------------------+
        */
        function updateAssessmentProperties($params) {

            $cnt = 0;
            /* set date format for any date assigned */
            $date_format = 'mm/dd/yyyy HH24:MI:SS';  //used by Oracle, ignored by Postgres

            foreach ($params as $key => $param) {
                /* filter out event id property and update others */
                if($key != 'assessment_id') {

                    $inParam = array();
                    $inParam[] = $params['assessment_id'];
                    $inParam[] = $key;
                    $inParam[] = str_replace("'","''",$param);
                    $inParam[] = $date_format;
                    $inParam[] = $this->db_transaction_username;
                    $rs = $this->funCall('mrs_assign_assessment_value_f', $inParam);

                    if( $this->isError($rs) ) {
                         return $this->raiseError(get_class($this)." ".$rs->message);
                    } elseif ($rs == 'collision_detected'){
                  return $rs;
                    }
               else{
               $cnt++;
                    }
                }
            }

            return $cnt;

      }
      /*
         +----------------------------------------------------------------------+
         | getAssessmentProperties                                     |
         +----------------------------------------------------------------------+
         | returns record set of the properties of the assessment               |
         | not including the response values                                    |
         | if error, returns error string                                       |
         +----------------------------------------------------------------------+
      */
      function getAssessmentProperties($assessment_id) {

               $qryString = "
                  SELECT
                     iv.label,
                     iv.salabel,
                     iv.display_type,
                     iv.version,
                     iv.copyright_notice,
                     iv.hide_sa_previous,
                     av.assessment_id,
                     av.instrument_id,
                     TO_CHAR(av.assessment_date, 'MM/DD/YYYY') AS assessment_date,
                     TO_CHAR(av.assessment_date, 'HH24:MI:SS') AS assessment_starttime,
                     av.ursi,
                     av.entry_code,
                     av.dataentry_type_id,
                     av.segment_interval,
                     av.segment_instance,
                     av.elapsed_time,
                     av.rater1,
                     av.rater2,
                     av.source_type,
                     av.site_id,
                     av.study_id,
                     av.successfully_completed,
                     av.missing_reason_id,
                     av.notes,
                     av.last_qid,
                     av.dx_excluded
                  FROM mrs_inst_vw iv, mrs_assessments_vw av
                  WHERE iv.instrument_id = av.instrument_id
                  AND av.assessment_id = $assessment_id
               ";


               /* simple select statement */
               $rs = $this->select($qryString);

               /* if return is error, return an error string to caller */
               if(PEAR::isError($rs)) {
                  return get_class($this)." getAssessmentProperties() ".$rs->getMessage();
               }
               /* return record set to caller */
               else {
                  return $rs;
               }

      }

    /*
       +-------------------------------------------------------------------+
       | getCalculatedFields()                                             |
       +-------------------------------------------------------------------+
       |                                                                   |
       +-------------------------------------------------------------------+
    */
    function getCalculatedFields($assessment_id) {
         $qryString = "
            SELECT question_id
            FROM mrsdba.mrs_inst_questions_lov_vw iqd
            INNER JOIN mrsdba.mrs_assessments_vw a
               ON a.instrument_id = iqd.instrument_id
               AND a.assessment_id = $assessment_id
            WHERE iqd.formula_question_type = 'Y'
            ORDER by iqd.sort_order
         ";
         $rs = $this->select($qryString);
         if ($this->isError($rs)) {
            return $this->raiseError(get_class($this)
               . ":" . __FUNCTION__ . " " . $rs->message);
         }
         return $rs;
      }
      /*********************************************************************
       * backfillQuestion
       * Updates all assessments entered against the $instrument_id
       * by adding the $question_id to them if they do not already
       * have the $question_id
       * If the assessments entered against the $instrument_id already have the
       * $question_id they will not be modified/affected
       **
       * 12/6/2012 Dylan Wood: Updated function to also accept the assessment_id
       * parameter. This is useful for backfilling all calculated fields upon
       * assessment completion.
       * 5/27/2013 Dylan Wood: Updated function to return count of backfilled
       * fields or PEAR error object. This allows each script that calls this
       * function to handle the result as needed.
       */
      function backfillQuestion(
         $instrument_id,
         $question_id,
         $assessment_id = 0
      ) {

         define("MAX_RESPONSE_VALUE_SIZE",200);
         require_once(BASE_FILE_PATH . '/asmt/classes/EvalQuestion.inc');

         $func = new EvalQuestion('',$instrument_id,$this);
         $func->setExpr($question_id);
         $qids = $func->extractQidsForBackfill($func->expr);
         $dep_qids = array();
         foreach ($qids as $qid) {
            $stripped = str_replace(array('[',']'),'',$qid);
            if (!in_array($stripped,$dep_qids))
               $dep_qids[] = $stripped;
         }
         $dep_qids_str = '{""';
         foreach ($dep_qids as $qid) {
            $dep_qids_str .= ',"' . $qid . '"';
         }
         $dep_qids_str .= '}';

         $inParam = array();
         $inParam[] = $instrument_id;
         $inParam[] = $question_id;
         $inParam[] = $dep_qids_str;
         $inParam[] = $assessment_id;

         $rs = $this->funCall('mrs_backfill_inst_question_f', $inParam);

         return $rs;
      }
      /*********************************************************************
       * backcalcQuestion
       * Updates all assessments entered against the $instrument_id
       * by recalculate the $question_id
       * If the assessments for $instrument_id do not already have the
       * $question_id they will not be modified/affected
       **
       * 12/6/2012 Dylan Wood: Updated function to also accept the
       * assessment_id parameter. This is useful for backfilling all
       * calculated fields upon assessment completion.
       */
      function backcalcQuestion(
         $instrument_id,
         $question_id,
         $assessment_id = 0
      ) {
         $return_array = array(
            'total_fields'=>0,
            'error_fields'=>0,
            'summary'=>array()
         );
         /* loop through all assessments that use $instrument_id */
         $qryString="
            SELECT
               r.assessment_id,
               r.question_instance
            FROM mrs_assessment_responses_vw r
            WHERE r.assessment_id IN (
               SELECT assessment_id
               FROM mrs_assessments_vw
               WHERE instrument_id = $instrument_id
         ";
         if ($assessment_id != 0) {
            $qryString.= " AND assessment_id = $assessment_id ";
         }
         $qryString .= "
            )
            AND question_id = '$question_id'
         ";

         $rs = $this->pointerSelect($qryString);

         /* if return is error, return an error string to caller */
         if (PEAR::isError($rs)) {
            return $rs;
         }
         $return_array['total_fields'] = 0;
         $return_array['error_fields'] = 0;

         $expr = new EvalQuestion('',$instrument_id,$this);
         if ($assessment_id != 0) {
            //just for a single assessment
            $expr->setExpr($question_id);
            $expr->setDepQIDs($expr->expr);
            $expr->setDepQIDsString($expr->expr);
            $expr->setDepQResponses($expr->depQIDsString, $assessment_id);
            $expr->setDepQAssumedVals($expr->depQIDsString);
         }
         while (1 === $rs->fetchInto($row, 0)) {
            $assessment_id = $row['assessment_id'];
            $question_instance = $row['question_instance'];
            $summary_str = $assessment_id . '[' . $question_id . ']'
               . '[' . $question_instance . ']';
            $res = $expr->evaluateAndUpdate(
               $row['assessment_id'],
               $question_id,
               $row['question_instance']
            );

            if (PEAR::isError($res)) {
               $summary_str .=
                  'Error calculating response. No value set: '
                  . $res->getMessage();
               $return_array['error_fields'] ++;
            }
            $summary_str .= ' = ' . $expr->result;
            if ($expr->errorMsg != '') {
               $return_array['error_fields'] ++;
               $summary_str .= '('.$expr->errorMsg.')';
            }
            $return_array['summary'][] = $summary_str;
            $return_array['total_fields']++;
         }
         return $return_array;
      }
    /*
       +-------------------------------------------------------------------+
       | writeAssessmentResponses()                                        |
       +-------------------------------------------------------------------+
       |                                                                   |
       +-------------------------------------------------------------------+
    */
    function writeAssessmentResponses($assessment_id, $aResponseValues) {
      $cnt = 0;
      /* traverse response array and insert each value */
      foreach ($aResponseValues as $questionID => $aResponses) {

        foreach ($aResponses as $instance => $responseValue) {

          $inParam = array();
          $inParam[] = $assessment_id;
          $inParam[] = $questionID;
          $inParam[] = $instance;
          $inParam[] = trim($responseValue['value']);
          $inParam[] = trim($responseValue['desc']);
          $inParam[]= $this->db_transaction_username;
          $rs = $this->funCall('mrs_assign_response_value_f',$inParam);

          if ($this->isError($rs)) {
               return $this->raiseError(get_class($this)
                  . " " . $rs->message);
          } else {
            $cnt++;
            if ($rs == 'INST_QUEST_MISMATCH') {
               $errorString = 'Warning!! The assessment you are editing '
                  . 'contains questions which do not belong to the active '
                  . 'instrument. Please contact Dylan Wood in Neuroinformatics '
                  . '(<a href="mailto:dwood@mrn.org"> dwood@mrn.org</a>), and '
                  . 'tell him that you have received this error';
               error_log('Instrument-Question-Conflict Detected! '
                  . 'Entry-user: ' . $this->db_transaction_username.' '
                  . 'Assessment_id: ' . $assessment_id.' '
                  . 'Question_id: ' . $questionID
               );
            }
          }
        }
      }
      if (isset($errorString)) {
         displayMsg(get_class($this) . " " . $errorString);
      }

      return $cnt;
    }
        /*
           +-----------------------------------------------------------+
           | completeAssessment()                                      |
           +-----------------------------------------------------------+
           |                                                           |
           +-----------------------------------------------------------+
        */
        function completeAssessment($assessment_id,$elapsedTime) {
            //First backfill assessment
            $calcFields = $this->getCalculatedFields($assessment_id);

            if (count($calcFields)) {
               $instrument_id = $this->select("
                  SELECT instrument_id
                  FROM mrsdba.mrs_assessments_vw
                  WHERE assessment_id = $assessment_id
               ");
               $instrument_id = $instrument_id[0]['instrument_id'];
               foreach ($calcFields as $calcField) {
                  $backfillResult = $this->backfillQuestion(
                     $instrument_id,
                     $calcField['question_id'],
                     $assessment_id
                  );
                  //only proceed if at least one question was backfilled
                  if ($backfillResult > 0) {
                     $this->backcalcQuestion(
                        $instrument_id,
                        $calcField['question_id'],
                        $assessment_id
                     );
                  }
               }
            }

            $inParam = array();
            $inParam[] = $assessment_id;
            $inParam[] = $this->db_transaction_username;

            $rs = $this->funCall('mrs_submit_self_assessment_f', $inParam);

            if ($this->isError($rs)) {
                 return $this->raiseError(get_class($this)
                    . " " . $rs->message);
            }

            return $rs;
        }
        /*
           +----------------------------------------------------------------------+
           | setAssessmentLastQID()                                                    |
           +----------------------------------------------------------------------+
           |                                            |
           +----------------------------------------------------------------------+
        */

      function setAssessmentLastQID($asmtID,$lastQID){
         $inParam = array();
         $inParam[] = $asmtID;
         $inParam[] = $lastQID;

         $rs = $this->funCall('mrs_update_asmt_last_qid_f', $inParam);

         if( $this->isError($rs) ) {
              return $this->raiseError(get_class($this)." ".$rs->message);
         }
      }
      function getCriticalResponses($asmt_id){
         $qry = "select distinct question_id from mrsdba.mrs_critical_responses_vw
                 where assessment_id=$asmt_id";

         $rs = $this->select($qry);

         $result = array();
         foreach($rs as $row){
            $result[] = $row['question_id'];
         }
         return $result;
      }
      function insertClientError($message, $src_url, $line_number, $page_url,
         $svjson, $user_note, $client_agent){

         $inParam = array();
         $inParam[] = $message;
         $inParam[] = $src_url;
         $inParam[] = $line_number;
         $inParam[] = $page_url;
         $inParam[] = $svjson;
         $inParam[] = $user_note;
         $inParam[] = $client_agent;
         $inParam[] = $this->db_transaction_username;

         $rs = $this->funCall('mrsdba.mrs_insert_client_error_f', $inParam);

         return $rs;

      }

      //event_types is an array of events
      function getAssessmentEvent($assessment_id, $event_types){
         $query = "select * from mrs_assessment_events_vw ev join mrs_assessment_event_types_vw ty using(event_type_id)
                  where assessment_id = $assessment_id ";
         if($event_types) $query.=" and ty.label in ('".implode("','",$event_types)."') ";
         $query.= " order by event_id";
         $rs = $this->select($query);
           if(PEAR::isError($rs)) {
               error_log('FILE: '.__FILE__.' LINE: '.__LINE__.' MESSAGE: '.$rs->getMessage());
               return array();
           }
           else return $rs;
      }

      //get all entries for one assessment, to check whether it has C entry code
      function getAllEntry41Asmt($assessment_id){
         $query = "SELECT ae1.assessment_id, ae1.entry_code
                  FROM mrs_assessments_vw ae1, mrs_assessments_vw ae2
               WHERE ae1.instrument_id = ae2.instrument_id
                     AND ae1.segment_interval = ae2.segment_interval
                     AND ae1.segment_instance = ae2.segment_instance
                     AND ae1.ursi = ae2.ursi
                     AND ae1.site_id = ae2.site_id
                     AND ae1.dataentry_type_id = ae2.dataentry_type_id
                     AND nvl(ae1.source_type,0) = nvl(ae2.source_type,0)
                     AND ae1.rater1 = ae2.rater1
                     AND ae2.assessment_id = $assessment_id";
         $rs = $this->select($query);
         if(PEAR::isError($rs)) {
               error_log('FILE: '.__FILE__.' LINE: '.__LINE__.' MESSAGE: '.$rs->getMessage());
               return array();
           }
           else return $rs;
      }

      /*
      *    Returns the SiteID of a subject
      */
      function getSiteFromSubjectID($subject_id){
         $qryStr = "SELECT s.site_id
                    FROM mrsdba.mrs_subject_logins_vw l
                    JOIN mrs_studies2_vw s using(study_id)
                    WHERE subject_id=?";

         $rs = $this->bindQuery($qryStr, array($subject_id));
         /* if return is error, return an error string to caller */
         if(PEAR::isError($rs)) {
            return get_class($this)." getLoginFromSubjectID():".$rs->getMessage();
         }
         return $rs[0]['site_id'];
      }


      function updateSubjectRacialCategories($usid, $post) {

         /* racial category */
         $subjects_race_cats=array();
         $racial_category_id = "";
         $racial_category_ids = "";

         foreach( $post as $key => $value) {

            if( trim(substr($key,0,6)) == 'racCat') {
               $racial_category_id = substr($key,6);
               $racial_category_ids .= $racial_category_id . ",";
            }
         }
         if( strlen($racial_category_ids) > 0 ) {
            if( substr($racial_category_ids,-1) == ",") {
               $racial_category_ids=substr(
                  $racial_category_ids,0,strlen($racial_category_ids)-1);
            }
         }

         if( $racial_category_ids =='')
            $racial_category_ids = null;
         $inParam = array();
         $inParam[] = revealUSID($usid);
         $inParam[] = revealURSI($post['ursi']);
         $inParam[] = $racial_category_ids;
         /* We use -9999 instead of NULL because
          * psql doesn't like NULLs.  subject_type_id
          * could be NULL if we are updating
          * the racial categories on a new subject.
          */
         $inParam[] = isset($post['subject_type_id']) ? $post['subject_type_id'] : -9999;
         $inParam[] = $this->db_transaction_username;

         $rs = $this->funCall('mrs_update_subject_race_cats_f', $inParam);

         if (PEAR::isError($rs)) {
              return $this->raiseError(get_class($this)
                 ." (updateSubjectRacialCategories) ".$rs->getMessage());
         }

         return $rs;
      }
      function getEthnicCategories() {
         $qryStr = "select config_label, config_value
                     from mrs_configs_vw where group_label = 'ETHNIC_CATEGORY'
         ";

         $rs = $this->select( $qryStr );

         return $rs;
      }

        function getRacialCategories() {
            $qryStr = "select racial_category_id, label
                            from mrs_racial_categories_vw
            ";

            $rs = $this->select( $qryStr );

            return $rs;
        }

        /*
           +----------------------------------------------------------------------+
           | add new subject                                                      |
           +----------------------------------------------------------------------+
           |                                            |
           +----------------------------------------------------------------------+
        */
        function addSubject($post, $bCheckMatchesBeforeAdd=false) {

            if (isset($post['agreestosharedata'])) {
                $agreestosharedata=$post['agreestosharedata'];
            } else {
                $agreestosharedata = "";
            }

            if( $bCheckMatchesBeforeAdd ){
                $addFunctionMatchLevel = "NORMAL";
            } else {
                $addFunctionMatchLevel = "NONE";
            }


            $sid = session_id();
            /* simplify reference so it will work in the function call below */
            $dob = $post['BirthDate'];

            $siteid= $post['site_id'];
            $first_name = $this->encryptAppString($post['FirstName']);
            $last_name = $this->encryptAppString($post['LastName']);
            $email = $post['userEmail'];
            $tag_site_id = null;
            $tag_study_id = null;
            if (isset($post['context']) && $post['context'] === 'site')
                $tag_site_id = $post['site_id'];
            if (isset($post['context']) && $post['context'] === 'study')
                $tag_study_id = $post['study_id'];

            $inParam = array();
            $inParam[] = $siteid;
            $inParam[] = $sid;
            $inParam[] = $addFunctionMatchLevel;
            if (!isset($post['subject_tag_id']) || $post['subject_tag_id']=='') {
                $inParam[] = NULL;
            } else {
                $inParam[] = $post['subject_tag_id'];
            }
            if (!isset($post['value'])) {
                $inParam[] =  null;
            } else {
                $inParam[] = ($post['value'] == null || trim($post['value']) == '')?
                    null : $this->encryptAppString($post['value']);
            }
            $inParam[] = $first_name;
            $inParam[] = $this->encryptAppString($post['MiddleName']);
            $inParam[] = $last_name;
            $inParam[] = $dob;
            $inParam[] = $post['Gender'];
            $inParam[] = $post['Line1'];
            $inParam[] = $post['Line2'];
            $inParam[] = $post['City'];
            $inParam[] = $post['State'];
            $inParam[] = $post['Zip'];
            $inParam[] = $post['Country'];
            $inParam[] = $post['Notes'];
            $inParam[] = $email;
            $inParam[] = AUTH_APP_ENCKEY;
            $inParam[] = AUTH_APP_ENCIV;
            $inParam[] = $this->encryptAppString($post['Suffix']);
            $inParam[] = $this->db_transaction_username;
                        //studies for roll, empty as subjects dont access studies
            $inParam[] = '';
            $inParam[] = $tag_site_id;
            $inParam[] = $tag_study_id;
            $inParam[] = $post['userPwd'];
            $inParam[] = 'true'; //is_p2_active
            $inParam[] = trim($post['first_name_at_birth']) === '' ? null : $this->encryptAppString(trim($post['first_name_at_birth']));
            $inParam[] = trim($post['middle_name_at_birth']) === '' ? null : $this->encryptAppString(trim($post['middle_name_at_birth']));
            $inParam[] = trim($post['last_name_at_birth']) === '' ? null : $this->encryptAppString(trim($post['last_name_at_birth']));
            $inParam[] = trim($post['physical_sex_at_birth']) === '' ? null : trim($post['physical_sex_at_birth']);
            $inParam[] = trim($post['city_born_in']) === '' ? null : $this->encryptAppString(trim($post['city_born_in']));
            
            require_once('classes/StudyORM.class.php');
            $study = StudyORM::factory(array('study_id' => $post['study_id']));
            if($this->isCurrentSubject(
                $first_name,
                $last_name,
                $dob)
                && !$study[0]->get('recruitment_study')
            ) {
                //subject already exists. return
                return 'subjectExists';
            } else if($this->isCurrentSubject(
                $first_name,
                $last_name,
                $dob,
                $study[0]->get('study_id'))
                && $study[0]->get('recruitment_study')
            ) {
                //subject already exists in the same study. return
                return 'subjectExists';
            } //elseif ($this->checkEmail($email)) {

                //return 'emailExists';
            //}

	        //$regex = "/^([A-Za-z0-9_\-\.])+\@([A-Za-z0-9_\-\.])+\.([A-Za-z]{2,4})$/";
            //if (!preg_match($regex, trim($this->decryptAppString($email)))) {
               //invalid email
               //return 'badEmail';
            //}
            $rs = $this->funCall('mrs_add_subject_f', $inParam);

            if (PEAR::isError($rs)) {
                return $this->raiseError(get_class($this)."-1019 ".$rs->toString() . "<BR>");
            }
            // push the names to the matching service
            $names = array();
            $names['first'] = $post['FirstName'];
            $names['last'] = $post['LastName'];
            $response = webserviceClient::callService('POST', MATCHING_SERVICE_URL, $names);
            if ($response->isError()) {
                error_log('ERROR nameMatcher: ' . $response->getErrorMessage());
            }

            if(isset($post['phone1_phone_num'])) {
                $response = $this->addSubjectPhone($rs,
                    $post['phone1_area_code'],
                    $post['phone1_phone_num'],
                    $post['phone1_extension'],
                    $post['phone1_notes']);
                if (PEAR::isError($response)) {
                    return $this->raiseError(get_class($this)." ".$response->getMessage());
                }
            }
            if(!empty($post['phone2_phone_num'])) {
                $response = $this->addSubjectPhone($rs,
                    $post['phone2_area_code'],
                    $post['phone2_phone_num'],
                    $post['phone2_extension'],
                    $post['phone2_notes']);
                if (PEAR::isError($response)) {
                    return $this->raiseError(get_class($this)." ".$response->getMessage());
                }
            }

            if(!empty($post['phone3_phone_num'])) {
                $response = $this->addSubjectPhone($rs,
                    $post['phone3_area_code'],
                    $post['phone3_phone_num'],
                    $post['phone3_extension'],
                    $post['phone3_notes']);
                if (PEAR::isError($rs)) {
                    return $this->raiseError(get_class($this)." ".$response->getMessage());
                }
            }
            return $rs;
        }

        function isCurrentSubject($first, $last, $dob, $studyId=null) {
            $param = array($first, $last, $dob);
            $query = "
                SELECT EXISTS (
                    SELECT s.email_address,s.first_name,s.last_name,s.birth_date
                    FROM mrs_subjects2_vw s "
                    . ($studyId === null ? 
                       "" : "INNER JOIN mrs_subject_details_active_vw ac ON s.usid = ac.usid")
                    . " WHERE s.first_name IN (?)
                    AND s.last_name IN (?)
                    AND s.birth_date IN (?)"
                    . ($studyId === null ? "" : (" AND ac.study_id =".$studyId)) 
                .");";
            $rs = $this->bindQuery($query, $param);
            if(PEAR::isError($rs)) {
                return $this->raiseError(get_class($this) .
                    " isCurrentSubject: ".$rs->getMessage());
            }
            return $rs[0]['?column?'] == 't' ? true:false;
        }

      function getSites($siteId=null,$orderByLabel=false) {
         $qryStr = "
            SELECT site_id,label,description,ursi_prefix
            FROM casdba.cas_sites_vw c
         ";

         if($siteId!=null) {
            $qryStr .= " WHERE site_id = '$siteId' ";
         }

         if ($orderByLabel) {
            $qryStr .= 'ORDER BY label';
         } else {
            $qryStr .= 'ORDER BY description';
         }

         $rs = $this->select( $qryStr );

         if(PEAR::isError($rs)) {
            return $this->raiseError( __FUNCTION__ . ": (" .__LINE__ .
              ") " . $rs->getMessage());
         }
         /* return recordset or PEAR error object */
         return $rs;
      }

      function getSiteConfig($site_id=null, $show_enroll=false) {

         $qryStr = "select * from casdba.cas_sites_vw c
                   inner join casdba.cas_sites_config_vw m
                   using(site_id)";
         if($site_id !== null) {
            $qryStr.= " where site_id='".$site_id. "'";

            if($show_enroll) {
               $qryStr .= " and allow_p2_self_enrollment = true";
            }
         } elseif($show_enroll) {
            $qryStr .= " where allow_p2_self_enrollment = true";
         }

         $rs = $this->select( $qryStr );

         if(PEAR::isError($rs)) {
            return $this->raiseError( __FUNCTION__ . ": (" .__LINE__ .
              ") " . $rs->getMessage());
         }
         /* return recordset or PEAR error object */
         if (isset($rs[0])) {
            if($site_id !== null ) {
               return $rs[0];
            } else {
               return $rs;
            }
         } else {
            return null;
         }
        }

            /*
           +----------------------------------------------------------------------+
           | add subject phone
           +----------------------------------------------------------------------+
           |
           +----------------------------------------------------------------------+
        */
        function addSubjectPhone($usid, $area_code, $phone_num, $extension, $notes) {

            $inParam = array();
            $inParam[] = $usid;
            $inParam[] = $area_code;
            $inParam[] = $phone_num;
            $inParam[] = $extension;
            $inParam[] = $notes;
            $rs = $this->funCall('MRS_ADD_SUBJECT_PHONE_F', $inParam);

            if (PEAR::isError($rs)) {
                 return $this->raiseError(get_class($this)." ".$rs->getMessage());
            }

            return $rs;
        }

        /*
           +---------------------------------------------------------------------+
           | enroll a subject                                                       |
           +---------------------------------------------------------------------+
           | returns: URSI or PEAR_Error Object                                  |
           +---------------------------------------------------------------------+
        */
    function enrollSubject($site_id,
                            $usid,
                            $subject_type_id,
                            $notes='',
                            $agreestosharedata="NULL",
                            $agrees_to_future_studies=null,
                            $consent_date,
                            $ethnicity) {

        /* prepare statement */
        $inParam = array();
        $inParam[] = $usid;
        $inParam[] = intval($subject_type_id);
        $inParam[] = str_replace("'","''",$notes);
        $inParam[] = $agreestosharedata;
        $inParam[] = $this->dbBoolEncode($agrees_to_future_studies);
        $inParam[] = $this->db_transaction_username;
        $inParam[] = $site_id;
        $inParam[] = $consent_date;
        $inParam[] = $ethnicity;
        $rs = $this->funCall('mrs_assign_subject_type_f', $inParam);

        if (PEAR::isError($rs)) {
            return $this->raiseError(get_class($this)."::".__FUNCTION__ . $rs->getMessage());
        }

        /* return ursi */
        return $rs;
    }

      function addSubjectLogin($usid, $study_id, $subject_id, $source_type) {

         // remove old queue
         $inParam = array();
         $inParam[] = $usid;
         $inParam[] = $study_id;
         $inParam[] = $subject_id;
         $inParam[] = $source_type;
         $inParam[] = $this->db_transaction_username;
         $rs = $this->funCall('mrs_add_subject_login_f', $inParam);

         return $rs;
      }

   function generateSubjectLogin() {
      $login = substr(md5(microtime()),rand(0,21),10);

      $query = "SELECT subject_id
               FROM mrs_subject_logins_vw
               WHERE subject_id = (?);";
      $rs = $this->bindQuery($query, array($login));

      if(!$rs) {
         return $login;
      } else {
         return $this->generateSubjectLogin();
      }
   }

   function getStudyFromSubjectType($subject_type_id) {
      $query = "SELECT DISTINCT study_id
                FROM mrs_subject_types_vw
                WHERE subject_type_id = (?)";

      $rs = $this->bindQuery($query, array($subject_type_id));
      if (PEAR::isError($rs)) {
         return $this->raiseError(get_class($this)." ".$rs->getMessage());
      }

      return empty($rs) ? $rs : $rs[0]['study_id'];
   }

   function getSiteFromSubjectType($subject_type_id) {
      $query = "SELECT DISTINCT site_id
                FROM mrs_subject_types_vw t
                INNER JOIN mrs_studies_vw s
                ON s.study_id = t.study_id
                WHERE subject_type_id = (?)";

      $rs = $this->bindQuery($query, array($subject_type_id));
      if (PEAR::isError($rs)) {
         return $this->raiseError(get_class($this)." ".$rs->getMessage());
      }

      return empty($rs) ? $rs : $rs[0]['site_id'];
   }

   function getSubjectURSI($usid, $study_id) {
      $qryStr = "
         SELECT ursi
         FROM mrs_subject_details_vw
         WHERE usid = '$usid'
         AND study_id = $study_id
      ";

      $rs = $this->select( $qryStr );
      if (PEAR::isError($rs)) {
         return $this->raiseError("Call to  " .
            __FUNCTION__." in ".__FILE__." at ".__LINE__.": " .
             $rs->getMessage());
       }

      /* return recordset or PEAR error object */
      return $rs[0];
   }

   function setNonstopQueue($login_id, $is_nonstop){
      $funcName='mrs_set_sa_nonstop_queue_f';
      $param=array($login_id, $is_nonstop, $this->db_transaction_username);
      $rs=$this->funCall($funcName, $param);
      if (PEAR::isError($rs)) {
          $this-raiseError("Call to  " .
             __FUNCTION__." in ".__FILE__." at ".__LINE__.": " .
             $rs->getMessage());
      }
      return $rs;
   }
    function getQuestionTableInfo($sectionID){
        $sectionTables = array();

        //get table row info
        $query = "select t.table_id, r.row_id, r.label as row_label
                    from mrs_instrument_tables_vw t
                        join mrs_instrument_table_rows_vw r using(table_id)
                        join mrs_inst_questions_vw q on r.row_id = q.table_row_id
                    where t.section_id = $sectionID and q.deletion_date IS NULL
                    order by t.table_id, r.sort_order";

        $rs = $_SESSION['objDbLink']->select($query);
        if(PEAR::isError($rs)) {
            error_log('FILE: '.__FILE__.' LINE: '.__LINE__.' MESSAGE: '.$rs->getMessage());
            return array();
        }

        while(list($rowNum, $row) = each($rs)){
           $tableId = $row['table_id'];
           if(!array_key_exists($tableId, $sectionTables)) {
               $sectionTables[$tableId]['rows'] = array();
               $sectionTables[$tableId]['cols'] = array();
           }
           $sectionTables[$tableId]['rows'][$row['row_id']] = $row['row_label'];
        }
        //get table col info
        $query = "select t.table_id, c.col_id, c.label as col_label
                    from mrs_instrument_tables_vw t
                        join mrs_instrument_table_cols_vw c using(table_id)
                        join mrs_inst_questions_vw q on c.col_id = q.table_col_id
                    where t.section_id = $sectionID and q.deletion_date IS NULL
                    order by t.table_id, c.sort_order";

        $rs = $_SESSION['objDbLink']->select($query);
        if(PEAR::isError($rs)) {
            error_log('FILE: '.__FILE__.' LINE: '.__LINE__.' MESSAGE: '.$rs->getMessage());
            return array();
        }
        while(list($rowNum, $row) = each($rs)){
           $tableId = $row['table_id'];
           $sectionTables[$tableId]['cols'][$row['col_id']] = $row['col_label'];
        }
        return $sectionTables;
    }

    function getQTableQuestionIDs($rowIds = array()){
        $questionIds = array();
        $query = "SELECT q.question_id, q.table_row_id, q.table_col_id
                    FROM mrs_inst_questions_vw q
                    WHERE deletion_date IS NULL
                        AND table_row_id in (".implode(',', $rowIds).")";
        $rs = $_SESSION['objDbLink']->select($query);
        if (PEAR::isError($rs)){
            error_log('FILE: '.__FILE__.' LINE: '.__LINE__.' MESSAGE: '.$rs->getMessage());
            return $questionIds;
        }
        while(list($rowNum, $row) = each($rs)){
            $questionIds[$row['table_row_id']][$row['table_col_id']] = $row['question_id'];
        }
        return $questionIds;
    }

   function checkStudyEnrollment($usid, $studyID) {
      $param = array();
      $param[] = $usid;
      $param[] = $studyID;
      $qryStr = "
         SELECT EXISTS (
            SELECT usid
            FROM mrs_subject_details_active_vw
            WHERE usid = ?
            AND study_id = ?
         );";

      $rs = $this->bindQuery($qryStr, $param);
      if (PEAR::isError($rs)) {
         return $this->raiseError("Call to  " .
            __FUNCTION__." in ".__FILE__." at ".__LINE__.": " .
             $rs->getMessage());
       }
       return $rs[0]['?column?'] == 't' ? true:false;
   }
        function checkEmail($email) {
            $param = array($email);
            $query = "
                SELECT EXISTS (
                    SELECT email_address
                    FROM mrs_subjects2_vw
                    WHERE email_address IN (?)
                );";

            $rs = $this->bindQuery($query, $param);
            if(PEAR::isError($rs)) {
                return $this->raiseError(get_class($this) .
                    " checkEmail: ".$rs->getMessage());
            }
            return $rs[0]['?column?'] == 't' ? true:false;
        }
}
?>
