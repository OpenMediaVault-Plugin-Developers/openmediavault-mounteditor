<?php
/**
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @copyright Copyright (c) 2024 openmediavault plugin developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
class OMVRpcServiceMountEditor extends \OMV\Rpc\ServiceAbstract
{
    public function getName()
    {
        return 'MountEditor';
    }

    public function initialize()
    {
        $this->registerMethod('list');
        $this->registerMethod('get');
        $this->registerMethod('set');

        $this->registerMethod('addQuota');
        $this->registerMethod('removeQuota');
    }

    public function list($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.common.getlist');
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $objects = $db->getByFilterAssoc('conf.system.filesystem.mountpoint', [
            'operator' => 'stringNotEquals',
            'arg0' => 'hidden',
            'arg1' => '1'
        ]);
        // Filter the result.
        return $this->applyFilter($objects, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']);
    }

    public function get($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.common.objectuuid');
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        return $db->getAssoc('conf.system.filesystem.mountpoint', $params['uuid']);
    }

    public function set($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.system.filesystem.mountpoint', $params['uuid']);
        $opts = explode(',', $params['opts']);
        $optst = array_map('trim', $opts);
        $optsu = array_unique($optst);
        $optsf = implode(',', array_filter($optsu));
        if (strlen($optsf) < 1) {
            throw new \OMV\Exception( gettext("The mount must have at least one mount option!"));
        }
        $object->set('opts', $optsf);
        $db->set($object);
        // Return the configuration object.
        return $object->getAssoc();
    }

    public function addQuota($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.system.filesystem.mountpoint', $params['uuid']);
        $opts = explode(',', $object->get('opts'));
        $optst = array_map('trim', $opts);
        $optadd = ['usrjquota=aquota.user', 'grpjquota=aquota.group', 'jqfmt=vfsv0'];
        $optsm = array_merge($optst, $optadd);
        $optsu = array_unique($optsm);
        $optsf = implode(',', array_filter($optsu));
        if (strlen($optsf) < 1) {
            throw new \OMV\Exception( gettext("The mount must have at least one mount option!"));
        }
        $object->set('opts', $optsf);
        $db->set($object);
        // Return the configuration object.
        return $object->getAssoc();
    }

    public function removeQuota($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get('conf.system.filesystem.mountpoint', $params['uuid']);
        $opts = explode(',', $object->get('opts'));
        $optst = array_map('trim', $opts);
        $optsu = array_unique($optst);
        $optremove = ['usrjquota=aquota.user', 'grpjquota=aquota.group', 'jqfmt=vfsv0'];
        $optsd = array_diff($optsu, $optremove);
        $optsf = implode(',', array_filter($optsd));
        if (strlen($optsf) < 1) {
            throw new \OMV\Exception( gettext("The mount must have at least one mount option!"));
        }
        $object->set('opts', $optsf);
        $db->set($object);
        // Return the configuration object.
        return $object->getAssoc();
    }
}
