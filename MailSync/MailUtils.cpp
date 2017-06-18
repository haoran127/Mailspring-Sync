//
//  MailUtils.cpp
//  MailSync
//
//  Created by Ben Gotow on 6/15/17.
//  Copyright © 2017 Foundry 376. All rights reserved.
//

#include "MailUtils.hpp"
#include "sha256.h"
#include "constants.h"

int MailUtils::compareEmails(void * a, void * b, void * context) {
    return ((mailcore::String*)a)->compare((mailcore::String*)b);
}

std::string MailUtils::timestampForTime(time_t time) {
    std::tm * ptm = std::localtime(&time);
    char buffer[32];
    std::strftime(buffer, 32, "%Y-%m-%d %H:%M:%S", ptm);
    return std::string(buffer);
}

std::string MailUtils::roleForFolder(mailcore::IMAPFolder * folder) {
    mailcore::IMAPFolderFlag flags = folder->flags();
    if (flags & mailcore::IMAPFolderFlagAll) {
        return "all";
    }
    if (flags & mailcore::IMAPFolderFlagSentMail) {
        return "sent";
    }
    if (flags & mailcore::IMAPFolderFlagDrafts) {
        return "drafts";
    }
    if (flags & mailcore::IMAPFolderFlagJunk) {
        return "spam";
    }
    if (flags & mailcore::IMAPFolderFlagSpam) {
        return "spam";
    }
    if (flags & mailcore::IMAPFolderFlagImportant) {
        return "important";
    }
    if (flags & mailcore::IMAPFolderFlagStarred) {
        return "starred";
    }
    if (flags & mailcore::IMAPFolderFlagInbox) {
        return "inbox";
    }
    
    std::string path = std::string(folder->path()->UTF8Characters());
    std::transform(path.begin(), path.end(), path.begin(), ::tolower);
    
    if (COMMON_FOLDER_NAMES.find(path) != COMMON_FOLDER_NAMES.end()) {
        return COMMON_FOLDER_NAMES[path];
    }
    return "";
}

std::vector<uint32_t> MailUtils::uidsOfIndexSet(mailcore::IndexSet * set) {
    std::vector<uint32_t> uids {};
    mailcore::Range * range = set->allRanges();
    for (int ii = 0; ii < set->rangesCount(); ii++) {
        for (int x = 0; x < range->length; x ++) {
            uids.push_back((uint32_t)(range->location + x));
        }
        range += sizeof(mailcore::Range *);
    }
    return uids;
}

std::vector<uint32_t> MailUtils::uidsOfArray(mailcore::Array * array) {
    std::vector<uint32_t> uids {};
    for (int ii = 0; ii < array->count(); ii++) {
        uids.push_back(((mailcore::IMAPMessage*)array->objectAtIndex(ii))->uid());
    }
    return uids;
}

std::string MailUtils::idForFolder(mailcore::IMAPFolder * folder) {
    std::vector<unsigned char> hash(32);
    std::string src_str = std::string(folder->path()->UTF8Characters());
    picosha2::hash256(src_str.begin(), src_str.end(), hash.begin(), hash.end());
    return picosha2::bytes_to_hex_string(hash.begin(), hash.end());
}

std::string MailUtils::idForMessage(mailcore::IMAPMessage * msg) {
    mailcore::Array * addresses = new mailcore::Array();
    addresses->addObjectsFromArray(msg->header()->to());
    addresses->addObjectsFromArray(msg->header()->cc());
    addresses->addObjectsFromArray(msg->header()->bcc());
    
    mailcore::Array * emails = new mailcore::Array();
    for (int i = 0; i < addresses->count(); i ++) {
        mailcore::Address * addr = (mailcore::Address*)addresses->objectAtIndex(i);
        emails->addObject(addr->mailbox());
    }
    
    emails->sortArray(compareEmails, NULL);
    
    mailcore::String * participants = emails->componentsJoinedByString(new mailcore::String(""));
    mailcore::String * messageID = msg->header()->messageID();
    mailcore::String * subject = msg->header()->subject();
    
    std::string src_str = "";//timestampForTime(msg->header()->date());
    if (subject) {
        src_str = src_str.append(subject->UTF8Characters());
    }
    src_str = src_str.append("-");
    src_str = src_str.append(participants->UTF8Characters());
    src_str = src_str.append("-");
    src_str = src_str.append(messageID->UTF8Characters());
    
    std::vector<unsigned char> hash(32);
    picosha2::hash256(src_str.begin(), src_str.end(), hash.begin(), hash.end());
    return picosha2::bytes_to_hex_string(hash.begin(), hash.end());
}

